#include "database.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QUuid>

namespace {

const QStringList kDefaultCategories = {
    QStringLiteral("Еда"),
    QStringLiteral("Транспорт"),
    QStringLiteral("Жилье"),
    QStringLiteral("Зарплата"),
    QStringLiteral("Развлечения"),
    QStringLiteral("Здоровье"),
    QStringLiteral("Образование")
};

QString defaultDatabasePath()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (appDataPath.isEmpty()) {
        appDataPath = QDir::homePath() + QStringLiteral("/MoneyShark");
    }

    QDir dir(appDataPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        appDataPath = QDir::current().absoluteFilePath(QStringLiteral("data"));
        QDir fallbackDir(appDataPath);
        fallbackDir.mkpath(QStringLiteral("."));
    }

    return QDir(appDataPath).absoluteFilePath(QStringLiteral("moneyshark.db"));
}

} // namespace

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager manager;
    return manager;
}

DatabaseManager::DatabaseManager()
{
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::initDatabase(const QString &databasePath)
{
    if (m_database.isOpen()) {
        return true;
    }

    const QString dbPath = databasePath.isEmpty() ? defaultDatabasePath() : databasePath;
    QFileInfo dbInfo(dbPath);
    QDir().mkpath(dbInfo.absolutePath());

    if (QSqlDatabase::contains(m_connectionName)) {
        m_database = QSqlDatabase::database(m_connectionName);
    } else {
        m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    }
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        qWarning() << "Не удалось открыть локальную базу:" << m_database.lastError().text();
        return false;
    }

    QSqlQuery pragma(m_database);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    return createAndMigrateTables();
}

void DatabaseManager::closeDatabase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DatabaseManager::useLocalMode()
{
    if (!initDatabase()) {
        return false;
    }
    m_mode = Mode::Local;
    m_client.clearSession();
    clearCurrentUser();
    return true;
}

bool DatabaseManager::useServerMode(const QString &host, quint16 port, QString *error)
{
    m_client.setEndpoint(host, port);
    if (!m_client.testConnection(error)) {
        return false;
    }
    m_mode = Mode::Server;
    clearCurrentUser();
    return true;
}

DatabaseManager::Mode DatabaseManager::mode() const
{
    return m_mode;
}

bool DatabaseManager::isServerMode() const
{
    return m_mode == Mode::Server;
}

QString DatabaseManager::connectionDescription() const
{
    if (isServerMode()) {
        return QString("Сервер %1:%2").arg(m_client.host()).arg(m_client.port());
    }
    return QStringLiteral("Автономная локальная база");
}

bool DatabaseManager::loginUser(const QString &login, const QString &password, QString *error)
{
    if (login.trimmed().isEmpty() || password.isEmpty()) {
        if (error) {
            *error = QStringLiteral("Введите логин и пароль.");
        }
        return false;
    }

    if (isServerMode()) {
        if (!m_client.login(login.trimmed(), password, error)) {
            return false;
        }
        m_currentUserId = m_client.userId();
        m_currentLogin = m_client.loginName();
        return true;
    }

    if (!initDatabase()) {
        if (error) {
            *error = QStringLiteral("Локальная база данных недоступна.");
        }
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT id, password_hash, salt FROM users WHERE login = :login"));
    query.bindValue(QStringLiteral(":login"), login.trimmed());
    if (!query.exec() || !query.next()) {
        if (error) {
            *error = QStringLiteral("Неверный логин или пароль.");
        }
        return false;
    }

    const QString expectedHash = query.value(1).toString();
    const QString salt = query.value(2).toString();
    if (hashPassword(password, salt) != expectedHash) {
        if (error) {
            *error = QStringLiteral("Неверный логин или пароль.");
        }
        return false;
    }

    m_currentUserId = query.value(0).toInt();
    m_currentLogin = login.trimmed();
    ensureDefaultCategories(m_currentUserId);
    return true;
}

bool DatabaseManager::registerUser(const QString &login, const QString &password, QString *error)
{
    if (login.trimmed().size() < 3) {
        if (error) {
            *error = QStringLiteral("Логин должен содержать минимум 3 символа.");
        }
        return false;
    }
    if (password.size() < 6) {
        if (error) {
            *error = QStringLiteral("Пароль должен содержать минимум 6 символов.");
        }
        return false;
    }

    if (isServerMode()) {
        return m_client.registerUser(login.trimmed(), password, error);
    }

    return createUser(login.trimmed(), password, error);
}

int DatabaseManager::userCount() const
{
    if (isServerMode()) {
        return -1;
    }
    if (!m_database.isOpen()) {
        return 0;
    }
    QSqlQuery query(QStringLiteral("SELECT COUNT(*) FROM users"), m_database);
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::currentUserId() const
{
    return isServerMode() ? m_client.userId() : m_currentUserId;
}

QString DatabaseManager::currentLogin() const
{
    return isServerMode() ? m_client.loginName() : m_currentLogin;
}

QString DatabaseManager::sessionToken() const
{
    return isServerMode() ? m_client.sessionToken() : QString();
}

bool DatabaseManager::hasAuthenticatedUser() const
{
    return currentUserId() > 0 && !currentLogin().isEmpty();
}

void DatabaseManager::clearCurrentUser()
{
    m_currentUserId = -1;
    m_currentLogin.clear();
}

bool DatabaseManager::saveOperation(const FinanceOperation &operation)
{
    if (!hasAuthenticatedUser()) {
        return false;
    }

    if (isServerMode()) {
        return m_client.addOperation(operation);
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO operations (user_id, type, category, amount, date, note)
        VALUES (:user_id, :type, :category, :amount, :date, :note)
    )");
    query.bindValue(QStringLiteral(":user_id"), m_currentUserId);
    query.bindValue(QStringLiteral(":type"), static_cast<int>(operation.type));
    query.bindValue(QStringLiteral(":category"), operation.category);
    query.bindValue(QStringLiteral(":amount"), operation.amount);
    query.bindValue(QStringLiteral(":date"), operation.date.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":note"), operation.note);

    if (!query.exec()) {
        qWarning() << "Не удалось сохранить операцию:" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<FinanceOperation> DatabaseManager::loadAllOperations()
{
    QVector<FinanceOperation> operations;
    if (!hasAuthenticatedUser()) {
        return operations;
    }

    if (isServerMode()) {
        return m_client.listOperations();
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, user_id, type, category, amount, date, note "
        "FROM operations WHERE user_id = :user_id ORDER BY id"));
    query.bindValue(QStringLiteral(":user_id"), m_currentUserId);
    if (!query.exec()) {
        qWarning() << "Не удалось загрузить операции:" << query.lastError().text();
        return operations;
    }

    while (query.next()) {
        FinanceOperation operation;
        operation.id = query.value(0).toInt();
        operation.userId = query.value(1).toInt();
        operation.type = static_cast<FinanceOperation::Type>(query.value(2).toInt());
        operation.category = query.value(3).toString();
        operation.amount = query.value(4).toDouble();
        operation.date = QDateTime::fromString(query.value(5).toString(), Qt::ISODate);
        operation.note = query.value(6).toString();
        if (!operation.date.isValid()) {
            operation.date = QDateTime::currentDateTime();
        }
        operations.append(operation);
    }
    return operations;
}

bool DatabaseManager::removeLastOperation()
{
    if (!hasAuthenticatedUser()) {
        return false;
    }
    if (isServerMode()) {
        return m_client.removeLastOperation();
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "DELETE FROM operations WHERE id = "
        "(SELECT MAX(id) FROM operations WHERE user_id = :user_id) AND user_id = :user_id"));
    query.bindValue(QStringLiteral(":user_id"), m_currentUserId);
    return query.exec();
}

bool DatabaseManager::clearAllOperations()
{
    if (!hasAuthenticatedUser()) {
        return false;
    }
    if (isServerMode()) {
        return m_client.clearOperations();
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM operations WHERE user_id = :user_id"));
    query.bindValue(QStringLiteral(":user_id"), m_currentUserId);
    return query.exec();
}

bool DatabaseManager::saveBalance(double balance)
{
    if (!hasAuthenticatedUser()) {
        return false;
    }
    if (isServerMode()) {
        return m_client.setBalance(balance);
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO balance (user_id, amount, updated_at)
        VALUES (:user_id, :amount, CURRENT_TIMESTAMP)
        ON CONFLICT(user_id) DO UPDATE SET
            amount = excluded.amount,
            updated_at = CURRENT_TIMESTAMP
    )");
    query.bindValue(QStringLiteral(":user_id"), m_currentUserId);
    query.bindValue(QStringLiteral(":amount"), balance);
    return query.exec();
}

double DatabaseManager::loadBalance()
{
    if (!hasAuthenticatedUser()) {
        return 0.0;
    }
    if (isServerMode()) {
        return m_client.getBalance();
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT amount FROM balance WHERE user_id = :user_id"));
    query.bindValue(QStringLiteral(":user_id"), m_currentUserId);
    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    }
    saveBalance(0.0);
    return 0.0;
}

bool DatabaseManager::saveCategory(const QString &category)
{
    const QString cleanCategory = category.trimmed();
    if (!hasAuthenticatedUser() || cleanCategory.isEmpty()) {
        return false;
    }
    if (isServerMode()) {
        return m_client.addCategory(cleanCategory);
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT OR IGNORE INTO categories (user_id, name) VALUES (:user_id, :name)"));
    query.bindValue(QStringLiteral(":user_id"), m_currentUserId);
    query.bindValue(QStringLiteral(":name"), cleanCategory);
    return query.exec();
}

QStringList DatabaseManager::loadAllCategories()
{
    if (!hasAuthenticatedUser()) {
        return {};
    }
    if (isServerMode()) {
        QStringList categories = m_client.listCategories();
        if (categories.isEmpty()) {
            for (const QString &category : kDefaultCategories) {
                m_client.addCategory(category);
            }
            categories = m_client.listCategories();
        }
        return categories;
    }

    ensureDefaultCategories(m_currentUserId);

    QStringList categories;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT name FROM categories WHERE user_id = :user_id ORDER BY name"));
    query.bindValue(QStringLiteral(":user_id"), m_currentUserId);
    if (query.exec()) {
        while (query.next()) {
            categories.append(query.value(0).toString());
        }
    }
    return categories;
}

bool DatabaseManager::clearAllCategories()
{
    if (!hasAuthenticatedUser()) {
        return false;
    }
    if (isServerMode()) {
        for (const QString &category : m_client.listCategories()) {
            Q_UNUSED(category);
        }
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM categories WHERE user_id = :user_id"));
    query.bindValue(QStringLiteral(":user_id"), m_currentUserId);
    return query.exec();
}

bool DatabaseManager::saveCreditRates(const QVector<RateRecord> &rates)
{
    return isServerMode() ? m_client.saveCreditRates(rates) : saveRatesToTable(QStringLiteral("credit_rates"), rates);
}

bool DatabaseManager::saveDepositRates(const QVector<RateRecord> &rates)
{
    return isServerMode() ? m_client.saveDepositRates(rates) : saveRatesToTable(QStringLiteral("deposit_rates"), rates);
}

bool DatabaseManager::saveCurrencyRates(const QVector<CurrencyRate> &rates)
{
    if (isServerMode()) {
        return m_client.saveCurrencyRates(rates);
    }
    if (!m_database.isOpen()) {
        return false;
    }

    QSqlQuery clearQuery(m_database);
    if (!clearQuery.exec(QStringLiteral("DELETE FROM currency_rates"))) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO currency_rates (code, name, value, source, fetched_at)
        VALUES (:code, :name, :value, :source, :fetched_at)
    )");
    for (const CurrencyRate &rate : rates) {
        query.bindValue(QStringLiteral(":code"), rate.code);
        query.bindValue(QStringLiteral(":name"), rate.name);
        query.bindValue(QStringLiteral(":value"), rate.value);
        query.bindValue(QStringLiteral(":source"), rate.source);
        query.bindValue(QStringLiteral(":fetched_at"), rate.fetchedAt);
        if (!query.exec()) {
            qWarning() << "Не удалось сохранить курс валют:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

QVector<RateRecord> DatabaseManager::loadCreditRates()
{
    return isServerMode() ? m_client.listCreditRates() : loadRatesFromTable(QStringLiteral("credit_rates"), QStringLiteral("rate ASC"));
}

QVector<RateRecord> DatabaseManager::loadDepositRates()
{
    return isServerMode() ? m_client.listDepositRates() : loadRatesFromTable(QStringLiteral("deposit_rates"), QStringLiteral("rate DESC"));
}

QVector<CurrencyRate> DatabaseManager::loadCurrencyRates()
{
    if (isServerMode()) {
        return m_client.listCurrencyRates();
    }

    QVector<CurrencyRate> rates;
    if (!m_database.isOpen()) {
        return rates;
    }

    QSqlQuery query(QStringLiteral("SELECT id, code, name, value, source, fetched_at FROM currency_rates ORDER BY code"), m_database);
    while (query.next()) {
        CurrencyRate rate;
        rate.id = query.value(0).toInt();
        rate.code = query.value(1).toString();
        rate.name = query.value(2).toString();
        rate.value = query.value(3).toDouble();
        rate.source = query.value(4).toString();
        rate.fetchedAt = query.value(5).toString();
        rates.append(rate);
    }
    return rates;
}

QString DatabaseManager::hashPassword(const QString &password, const QString &salt)
{
    const QByteArray data = (salt + QStringLiteral(":") + password).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

QString DatabaseManager::makeSalt()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool DatabaseManager::createAndMigrateTables()
{
    if (!createCoreTables()) {
        return false;
    }

    const int adminId = ensureDefaultAdmin();
    if (adminId <= 0) {
        return false;
    }

    if (tableExists(QStringLiteral("operations")) && !tableColumns(QStringLiteral("operations")).contains(QStringLiteral("user_id"))) {
        if (!execSql(QStringLiteral("ALTER TABLE operations RENAME TO operations_old"))) {
            return false;
        }
    }
    if (tableExists(QStringLiteral("balance")) && !tableColumns(QStringLiteral("balance")).contains(QStringLiteral("user_id"))) {
        if (!execSql(QStringLiteral("ALTER TABLE balance RENAME TO balance_old"))) {
            return false;
        }
    }
    if (tableExists(QStringLiteral("categories")) && !tableColumns(QStringLiteral("categories")).contains(QStringLiteral("user_id"))) {
        if (!execSql(QStringLiteral("ALTER TABLE categories RENAME TO categories_old"))) {
            return false;
        }
    }

    if (!createCoreTables()) {
        return false;
    }

    if (tableExists(QStringLiteral("operations_old"))) {
        QSqlQuery query(m_database);
        query.prepare(QStringLiteral(
            "INSERT INTO operations (user_id, type, category, amount, date, note, created_at) "
            "SELECT :user_id, type, category, amount, date, note, COALESCE(created_at, CURRENT_TIMESTAMP) FROM operations_old"));
        query.bindValue(QStringLiteral(":user_id"), adminId);
        if (!query.exec()) {
            qWarning() << "Не удалось перенести старые операции:" << query.lastError().text();
            return false;
        }
        execSql(QStringLiteral("DROP TABLE operations_old"));
    }

    if (tableExists(QStringLiteral("balance_old"))) {
        QSqlQuery query(m_database);
        query.prepare(QStringLiteral(
            "INSERT INTO balance (user_id, amount, updated_at) "
            "SELECT :user_id, amount, COALESCE(updated_at, CURRENT_TIMESTAMP) FROM balance_old ORDER BY rowid LIMIT 1"));
        query.bindValue(QStringLiteral(":user_id"), adminId);
        if (!query.exec()) {
            qWarning() << "Не удалось перенести старый баланс:" << query.lastError().text();
            return false;
        }
        execSql(QStringLiteral("DROP TABLE balance_old"));
    }

    if (tableExists(QStringLiteral("categories_old"))) {
        QSqlQuery query(m_database);
        query.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO categories (user_id, name, created_at) "
            "SELECT :user_id, name, COALESCE(created_at, CURRENT_TIMESTAMP) FROM categories_old"));
        query.bindValue(QStringLiteral(":user_id"), adminId);
        if (!query.exec()) {
            qWarning() << "Не удалось перенести старые категории:" << query.lastError().text();
            return false;
        }
        execSql(QStringLiteral("DROP TABLE categories_old"));
    }

    return ensureDefaultCategories(adminId);
}

bool DatabaseManager::createCoreTables()
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

bool DatabaseManager::tableExists(const QString &tableName) const
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'table' AND name = :name"));
    query.bindValue(QStringLiteral(":name"), tableName);
    return query.exec() && query.next();
}

QStringList DatabaseManager::tableColumns(const QString &tableName) const
{
    QStringList columns;
    QSqlQuery query(m_database);
    query.exec(QString("PRAGMA table_info(%1)").arg(tableName));
    while (query.next()) {
        columns.append(query.value(1).toString());
    }
    return columns;
}

bool DatabaseManager::execSql(const QString &sql) const
{
    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        qWarning() << "Ошибка SQL:" << query.lastError().text() << sql;
        return false;
    }
    return true;
}

int DatabaseManager::ensureDefaultAdmin()
{
    int id = findUserId(QStringLiteral("admin"));
    if (id > 0) {
        return id;
    }

    QString error;
    if (!createUser(QStringLiteral("admin"), QStringLiteral("admin123"), &error)) {
        qWarning() << error;
        return -1;
    }
    return findUserId(QStringLiteral("admin"));
}

int DatabaseManager::findUserId(const QString &login) const
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT id FROM users WHERE login = :login"));
    query.bindValue(QStringLiteral(":login"), login);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

bool DatabaseManager::createUser(const QString &login, const QString &password, QString *error)
{
    if (!m_database.isOpen()) {
        if (error) {
            *error = QStringLiteral("База данных недоступна.");
        }
        return false;
    }

    if (userCount() >= 10) {
        if (error) {
            *error = QStringLiteral("Регистрация запрещена: уже создано 10 пользователей.");
        }
        return false;
    }

    const QString salt = makeSalt();
    const QString passwordHash = hashPassword(password, salt);
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO users (login, password_hash, salt) VALUES (:login, :password_hash, :salt)"));
    query.bindValue(QStringLiteral(":login"), login);
    query.bindValue(QStringLiteral(":password_hash"), passwordHash);
    query.bindValue(QStringLiteral(":salt"), salt);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().nativeErrorCode() == QStringLiteral("2067")
                ? QStringLiteral("Пользователь с таким логином уже существует.")
                : QString("Не удалось создать пользователя: %1").arg(query.lastError().text());
        }
        return false;
    }

    const int userId = findUserId(login);
    if (userId > 0) {
        QSqlQuery balanceQuery(m_database);
        balanceQuery.prepare(QStringLiteral("INSERT OR IGNORE INTO balance (user_id, amount) VALUES (:user_id, 0)"));
        balanceQuery.bindValue(QStringLiteral(":user_id"), userId);
        balanceQuery.exec();
        ensureDefaultCategories(userId);
    }
    return true;
}

bool DatabaseManager::ensureDefaultCategories(int userId)
{
    if (userId <= 0 || !m_database.isOpen()) {
        return false;
    }

    QSqlQuery countQuery(m_database);
    countQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM categories WHERE user_id = :user_id"));
    countQuery.bindValue(QStringLiteral(":user_id"), userId);
    if (countQuery.exec() && countQuery.next() && countQuery.value(0).toInt() > 0) {
        return true;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT OR IGNORE INTO categories (user_id, name) VALUES (:user_id, :name)"));
    for (const QString &category : kDefaultCategories) {
        query.bindValue(QStringLiteral(":user_id"), userId);
        query.bindValue(QStringLiteral(":name"), category);
        if (!query.exec()) {
            qWarning() << "Не удалось создать категорию:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool DatabaseManager::saveRatesToTable(const QString &tableName, const QVector<RateRecord> &rates)
{
    if (!m_database.isOpen()) {
        return false;
    }

    QSqlQuery clearQuery(m_database);
    if (!clearQuery.exec(QString("DELETE FROM %1").arg(tableName))) {
        qWarning() << "Не удалось очистить таблицу ставок:" << clearQuery.lastError().text();
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QString("INSERT INTO %1 (bank, rate, description, source, fetched_at) "
                          "VALUES (:bank, :rate, :description, :source, :fetched_at)").arg(tableName));
    for (const RateRecord &rate : rates) {
        query.bindValue(QStringLiteral(":bank"), rate.bank);
        query.bindValue(QStringLiteral(":rate"), rate.rate);
        query.bindValue(QStringLiteral(":description"), rate.description);
        query.bindValue(QStringLiteral(":source"), rate.source);
        query.bindValue(QStringLiteral(":fetched_at"), rate.fetchedAt);
        if (!query.exec()) {
            qWarning() << "Не удалось сохранить ставку:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

QVector<RateRecord> DatabaseManager::loadRatesFromTable(const QString &tableName, const QString &orderBy)
{
    QVector<RateRecord> rates;
    if (!m_database.isOpen()) {
        return rates;
    }

    QSqlQuery query(QString("SELECT id, bank, rate, description, source, fetched_at FROM %1 ORDER BY %2")
                        .arg(tableName, orderBy),
                    m_database);
    while (query.next()) {
        RateRecord rate;
        rate.id = query.value(0).toInt();
        rate.bank = query.value(1).toString();
        rate.rate = query.value(2).toDouble();
        rate.description = query.value(3).toString();
        rate.source = query.value(4).toString();
        rate.fetchedAt = query.value(5).toString();
        rates.append(rate);
    }
    return rates;
}
