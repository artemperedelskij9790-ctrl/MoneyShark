#include "moneysharkserver.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QTcpSocket>
#include <QUuid>
#include <QVariant>
#include <QDebug>

namespace {

QString hashPassword(const QString &password, const QString &salt)
{
    return QString::fromLatin1(QCryptographicHash::hash((salt + QStringLiteral(":") + password).toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString makeSalt()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QJsonObject sqlOperationToJson(const QSqlQuery &query)
{
    QJsonObject object;
    object["id"] = query.value(0).toInt();
    object["user_id"] = query.value(1).toInt();
    object["type"] = query.value(2).toInt();
    object["category"] = query.value(3).toString();
    object["amount"] = query.value(4).toDouble();
    object["date"] = query.value(5).toString();
    object["note"] = query.value(6).toString();
    return object;
}

const QStringList kDefaultCategories = {
    QStringLiteral("Еда"),
    QStringLiteral("Транспорт"),
    QStringLiteral("Жилье"),
    QStringLiteral("Зарплата"),
    QStringLiteral("Развлечения"),
    QStringLiteral("Здоровье"),
    QStringLiteral("Образование")
};

} // namespace

MoneySharkServer::MoneySharkServer(QObject *parent)
    : QTcpServer(parent)
{
    connect(this, &QTcpServer::newConnection, this, &MoneySharkServer::acceptClient);
}

bool MoneySharkServer::start(quint16 port, const QString &databasePath)
{
    if (!initDatabase(databasePath)) {
        return false;
    }
    if (!listen(QHostAddress::Any, port)) {
        qCritical() << "Не удалось открыть порт" << port << errorString();
        return false;
    }
    qInfo() << "MoneySharkServer слушает порт" << serverPort();
    qInfo() << "База данных:" << m_database.databaseName();
    return true;
}

void MoneySharkServer::acceptClient()
{
    while (hasPendingConnections()) {
        QTcpSocket *socket = nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            while (socket->canReadLine()) {
                const QByteArray line = socket->readLine().trimmed();
                QJsonParseError parseError;
                const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
                QJsonObject response;
                if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                    response = makeError(QStringLiteral("Некорректный JSON-запрос."));
                } else {
                    response = handleRequest(document.object());
                }
                socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact));
                socket->write("\n");
                socket->flush();
                socket->disconnectFromHost();
            }
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

bool MoneySharkServer::initDatabase(const QString &databasePath)
{
    const QString connectionName = QStringLiteral("moneyshark_server_connection");
    if (QSqlDatabase::contains(connectionName)) {
        m_database = QSqlDatabase::database(connectionName);
    } else {
        m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    }

    const QString path = databasePath.isEmpty()
        ? QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("moneyshark_server.db"))
        : databasePath;
    QDir().mkpath(QFileInfo(path).absolutePath());
    m_database.setDatabaseName(path);
    if (!m_database.open()) {
        qCritical() << "Не удалось открыть базу сервера:" << m_database.lastError().text();
        return false;
    }
    QSqlQuery pragma(m_database);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    return createTables() && ensureAdminUser() > 0;
}

bool MoneySharkServer::createTables()
{
    return execSql(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            login TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )")
    && execSql(R"(
        CREATE TABLE IF NOT EXISTS operations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            type INTEGER NOT NULL,
            category TEXT NOT NULL,
            amount REAL NOT NULL,
            date TEXT NOT NULL,
            note TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES users(id)
        )
    )")
    && execSql(R"(
        CREATE TABLE IF NOT EXISTS balance (
            user_id INTEGER PRIMARY KEY,
            amount REAL NOT NULL DEFAULT 0,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES users(id)
        )
    )")
    && execSql(R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(user_id, name),
            FOREIGN KEY(user_id) REFERENCES users(id)
        )
    )")
    && execSql(R"(
        CREATE TABLE IF NOT EXISTS credit_rates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            bank TEXT NOT NULL,
            rate REAL NOT NULL,
            description TEXT,
            source TEXT,
            fetched_at TEXT NOT NULL
        )
    )")
    && execSql(R"(
        CREATE TABLE IF NOT EXISTS deposit_rates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            bank TEXT NOT NULL,
            rate REAL NOT NULL,
            description TEXT,
            source TEXT,
            fetched_at TEXT NOT NULL
        )
    )")
    && execSql(R"(
        CREATE TABLE IF NOT EXISTS currency_rates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            code TEXT NOT NULL,
            name TEXT NOT NULL,
            value REAL NOT NULL,
            source TEXT,
            fetched_at TEXT NOT NULL
        )
    )");
}

bool MoneySharkServer::execSql(const QString &sql)
{
    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        qWarning() << "SQL ошибка:" << query.lastError().text();
        return false;
    }
    return true;
}

int MoneySharkServer::userCount()
{
    QSqlQuery query(QStringLiteral("SELECT COUNT(*) FROM users"), m_database);
    return query.next() ? query.value(0).toInt() : 0;
}

int MoneySharkServer::findUserId(const QString &login)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT id FROM users WHERE login = :login"));
    query.bindValue(QStringLiteral(":login"), login);
    return query.exec() && query.next() ? query.value(0).toInt() : -1;
}

int MoneySharkServer::ensureAdminUser()
{
    int id = findUserId(QStringLiteral("admin"));
    if (id > 0) {
        ensureDefaultCategories(id);
        return id;
    }
    QString error;
    if (!createUser(QStringLiteral("admin"), QStringLiteral("admin123"), &error)) {
        qCritical() << error;
        return -1;
    }
    return findUserId(QStringLiteral("admin"));
}

bool MoneySharkServer::createUser(const QString &login, const QString &password, QString *error)
{
    if (userCount() >= 10) {
        if (error) {
            *error = QStringLiteral("Регистрация запрещена: уже создано 10 пользователей.");
        }
        return false;
    }
    if (findUserId(login) > 0) {
        if (error) {
            *error = QStringLiteral("Пользователь с таким логином уже существует.");
        }
        return false;
    }

    const QString salt = makeSalt();
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT INTO users (login, password_hash, salt) VALUES (:login, :hash, :salt)"));
    query.bindValue(QStringLiteral(":login"), login);
    query.bindValue(QStringLiteral(":hash"), hashPassword(password, salt));
    query.bindValue(QStringLiteral(":salt"), salt);
    if (!query.exec()) {
        if (error) {
            *error = QString("Не удалось создать пользователя: %1").arg(query.lastError().text());
        }
        return false;
    }

    const int userId = findUserId(login);
    QSqlQuery balanceQuery(m_database);
    balanceQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO balance (user_id, amount) VALUES (:user_id, 0)"));
    balanceQuery.bindValue(QStringLiteral(":user_id"), userId);
    balanceQuery.exec();
    ensureDefaultCategories(userId);
    return true;
}

bool MoneySharkServer::checkPassword(const QString &login, const QString &password, int *userId)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT id, password_hash, salt FROM users WHERE login = :login"));
    query.bindValue(QStringLiteral(":login"), login);
    if (!query.exec() || !query.next()) {
        return false;
    }
    if (hashPassword(password, query.value(2).toString()) != query.value(1).toString()) {
        return false;
    }
    if (userId) {
        *userId = query.value(0).toInt();
    }
    return true;
}

void MoneySharkServer::ensureDefaultCategories(int userId)
{
    QSqlQuery countQuery(m_database);
    countQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM categories WHERE user_id = :user_id"));
    countQuery.bindValue(QStringLiteral(":user_id"), userId);
    if (countQuery.exec() && countQuery.next() && countQuery.value(0).toInt() > 0) {
        return;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT OR IGNORE INTO categories (user_id, name) VALUES (:user_id, :name)"));
    for (const QString &category : kDefaultCategories) {
        query.bindValue(QStringLiteral(":user_id"), userId);
        query.bindValue(QStringLiteral(":name"), category);
        query.exec();
    }
}

QJsonObject MoneySharkServer::handleRequest(const QJsonObject &request)
{
    const QString command = request.value("command").toString();
    if (command == QStringLiteral("ping")) {
        return makeOk();
    }
    if (command == QStringLiteral("login")) {
        int userId = -1;
        if (!checkPassword(request.value("login").toString().trimmed(), request.value("password").toString(), &userId)) {
            return makeError(QStringLiteral("Неверный логин или пароль."));
        }
        const QString token = makeToken(userId);
        QJsonObject response = makeOk();
        response["session_token"] = token;
        response["user_id"] = userId;
        return response;
    }
    if (command == QStringLiteral("register")) {
        const QString login = request.value("login").toString().trimmed();
        const QString password = request.value("password").toString();
        if (login.size() < 3 || password.size() < 6) {
            return makeError(QStringLiteral("Логин должен быть от 3 символов, пароль - от 6 символов."));
        }
        QString error;
        return createUser(login, password, &error) ? makeOk() : makeError(error);
    }

    const int userId = authenticatedUserId(request);
    if (userId <= 0) {
        return makeError(QStringLiteral("Недействительная сессия. Выполните вход заново."));
    }

    if (command == QStringLiteral("getBalance")) return getBalance(userId);
    if (command == QStringLiteral("setBalance")) return setBalance(userId, request);
    if (command == QStringLiteral("addOperation")) return addOperation(userId, request);
    if (command == QStringLiteral("listOperations")) return listOperations(userId);
    if (command == QStringLiteral("removeLastOperation")) return removeLastOperation(userId);
    if (command == QStringLiteral("clearOperations")) return clearOperations(userId);
    if (command == QStringLiteral("listCategories")) return listCategories(userId);
    if (command == QStringLiteral("addCategory")) return addCategory(userId, request);
    if (command == QStringLiteral("listCreditRates")) return listRates(QStringLiteral("credit_rates"), QStringLiteral("rate ASC"));
    if (command == QStringLiteral("listDepositRates")) return listRates(QStringLiteral("deposit_rates"), QStringLiteral("rate DESC"));
    if (command == QStringLiteral("listCurrencyRates")) return listCurrencyRates();
    if (command == QStringLiteral("saveCreditRates")) return saveRates(QStringLiteral("credit_rates"), request);
    if (command == QStringLiteral("saveDepositRates")) return saveRates(QStringLiteral("deposit_rates"), request);
    if (command == QStringLiteral("saveCurrencyRates")) return saveCurrencyRates(request);

    return makeError(QString("Неизвестная команда: %1").arg(command));
}

int MoneySharkServer::authenticatedUserId(const QJsonObject &request) const
{
    return m_sessions.value(request.value("session_token").toString(), -1);
}

QJsonObject MoneySharkServer::makeError(const QString &error) const
{
    QJsonObject response;
    response["ok"] = false;
    response["error"] = error;
    return response;
}

QJsonObject MoneySharkServer::makeOk() const
{
    QJsonObject response;
    response["ok"] = true;
    return response;
}

QString MoneySharkServer::makeToken(int userId)
{
    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_sessions.insert(token, userId);
    return token;
}

QJsonObject MoneySharkServer::getBalance(int userId)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT amount FROM balance WHERE user_id = :user_id"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    QJsonObject response = makeOk();
    response["balance"] = query.exec() && query.next() ? query.value(0).toDouble() : 0.0;
    return response;
}

QJsonObject MoneySharkServer::setBalance(int userId, const QJsonObject &request)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO balance (user_id, amount, updated_at)
        VALUES (:user_id, :amount, CURRENT_TIMESTAMP)
        ON CONFLICT(user_id) DO UPDATE SET amount = excluded.amount, updated_at = CURRENT_TIMESTAMP
    )");
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":amount"), request.value("amount").toDouble());
    return query.exec() ? makeOk() : makeError(query.lastError().text());
}

QJsonObject MoneySharkServer::addOperation(int userId, const QJsonObject &request)
{
    const QJsonObject operation = request.value("operation").toObject();
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO operations (user_id, type, category, amount, date, note)
        VALUES (:user_id, :type, :category, :amount, :date, :note)
    )");
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":type"), operation.value("type").toInt());
    query.bindValue(QStringLiteral(":category"), operation.value("category").toString());
    query.bindValue(QStringLiteral(":amount"), operation.value("amount").toDouble());
    query.bindValue(QStringLiteral(":date"), operation.value("date").toString(QDateTime::currentDateTime().toString(Qt::ISODate)));
    query.bindValue(QStringLiteral(":note"), operation.value("note").toString());
    return query.exec() ? makeOk() : makeError(query.lastError().text());
}

QJsonObject MoneySharkServer::listOperations(int userId)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT id, user_id, type, category, amount, date, note FROM operations WHERE user_id = :user_id ORDER BY id"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    QJsonObject response = makeOk();
    QJsonArray array;
    if (query.exec()) {
        while (query.next()) {
            array.append(sqlOperationToJson(query));
        }
    }
    response["operations"] = array;
    return response;
}

QJsonObject MoneySharkServer::removeLastOperation(int userId)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM operations WHERE id = (SELECT MAX(id) FROM operations WHERE user_id = :user_id) AND user_id = :user_id"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    return query.exec() ? makeOk() : makeError(query.lastError().text());
}

QJsonObject MoneySharkServer::clearOperations(int userId)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM operations WHERE user_id = :user_id"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    return query.exec() ? makeOk() : makeError(query.lastError().text());
}

QJsonObject MoneySharkServer::listCategories(int userId)
{
    ensureDefaultCategories(userId);
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT name FROM categories WHERE user_id = :user_id ORDER BY name"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    QJsonArray array;
    if (query.exec()) {
        while (query.next()) {
            array.append(query.value(0).toString());
        }
    }
    QJsonObject response = makeOk();
    response["categories"] = array;
    return response;
}

QJsonObject MoneySharkServer::addCategory(int userId, const QJsonObject &request)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT OR IGNORE INTO categories (user_id, name) VALUES (:user_id, :name)"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":name"), request.value("category").toString().trimmed());
    return query.exec() ? makeOk() : makeError(query.lastError().text());
}

QJsonObject MoneySharkServer::listRates(const QString &tableName, const QString &orderBy)
{
    QSqlQuery query(QString("SELECT id, bank, rate, description, source, fetched_at FROM %1 ORDER BY %2").arg(tableName, orderBy), m_database);
    QJsonArray array;
    while (query.next()) {
        QJsonObject object;
        object["id"] = query.value(0).toInt();
        object["bank"] = query.value(1).toString();
        object["rate"] = query.value(2).toDouble();
        object["description"] = query.value(3).toString();
        object["source"] = query.value(4).toString();
        object["fetched_at"] = query.value(5).toString();
        array.append(object);
    }
    QJsonObject response = makeOk();
    response["rates"] = array;
    return response;
}

QJsonObject MoneySharkServer::saveRates(const QString &tableName, const QJsonObject &request)
{
    QSqlQuery clearQuery(m_database);
    if (!clearQuery.exec(QString("DELETE FROM %1").arg(tableName))) {
        return makeError(clearQuery.lastError().text());
    }

    QSqlQuery query(m_database);
    query.prepare(QString("INSERT INTO %1 (bank, rate, description, source, fetched_at) VALUES (:bank, :rate, :description, :source, :fetched_at)").arg(tableName));
    const QJsonArray rates = request.value("rates").toArray();
    for (const QJsonValue &value : rates) {
        const QJsonObject object = value.toObject();
        query.bindValue(QStringLiteral(":bank"), object.value("bank").toString());
        query.bindValue(QStringLiteral(":rate"), object.value("rate").toDouble());
        query.bindValue(QStringLiteral(":description"), object.value("description").toString());
        query.bindValue(QStringLiteral(":source"), object.value("source").toString());
        query.bindValue(QStringLiteral(":fetched_at"), object.value("fetched_at").toString(QDateTime::currentDateTime().toString(Qt::ISODate)));
        if (!query.exec()) {
            return makeError(query.lastError().text());
        }
    }
    return makeOk();
}

QJsonObject MoneySharkServer::listCurrencyRates()
{
    QSqlQuery query(QStringLiteral("SELECT id, code, name, value, source, fetched_at FROM currency_rates ORDER BY code"), m_database);
    QJsonArray array;
    while (query.next()) {
        QJsonObject object;
        object["id"] = query.value(0).toInt();
        object["code"] = query.value(1).toString();
        object["name"] = query.value(2).toString();
        object["value"] = query.value(3).toDouble();
        object["source"] = query.value(4).toString();
        object["fetched_at"] = query.value(5).toString();
        array.append(object);
    }
    QJsonObject response = makeOk();
    response["rates"] = array;
    return response;
}

QJsonObject MoneySharkServer::saveCurrencyRates(const QJsonObject &request)
{
    QSqlQuery clearQuery(m_database);
    if (!clearQuery.exec(QStringLiteral("DELETE FROM currency_rates"))) {
        return makeError(clearQuery.lastError().text());
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT INTO currency_rates (code, name, value, source, fetched_at) VALUES (:code, :name, :value, :source, :fetched_at)"));
    const QJsonArray rates = request.value("rates").toArray();
    for (const QJsonValue &value : rates) {
        const QJsonObject object = value.toObject();
        query.bindValue(QStringLiteral(":code"), object.value("code").toString());
        query.bindValue(QStringLiteral(":name"), object.value("name").toString());
        query.bindValue(QStringLiteral(":value"), object.value("value").toDouble());
        query.bindValue(QStringLiteral(":source"), object.value("source").toString());
        query.bindValue(QStringLiteral(":fetched_at"), object.value("fetched_at").toString(QDateTime::currentDateTime().toString(Qt::ISODate)));
        if (!query.exec()) {
            return makeError(query.lastError().text());
        }
    }
    return makeOk();
}
