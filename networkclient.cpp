#include "networkclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpSocket>

namespace {

QJsonObject operationToJson(const FinanceOperation &operation)
{
    QJsonObject object;
    object["id"] = operation.id;
    object["type"] = static_cast<int>(operation.type);
    object["category"] = operation.category;
    object["amount"] = operation.amount;
    object["date"] = operation.date.toString(Qt::ISODate);
    object["note"] = operation.note;
    return object;
}

FinanceOperation operationFromJson(const QJsonObject &object)
{
    FinanceOperation operation;
    operation.id = object.value("id").toInt(-1);
    operation.userId = object.value("user_id").toInt(-1);
    operation.type = static_cast<FinanceOperation::Type>(object.value("type").toInt(0));
    operation.category = object.value("category").toString();
    operation.amount = object.value("amount").toDouble();
    operation.date = QDateTime::fromString(object.value("date").toString(), Qt::ISODate);
    operation.note = object.value("note").toString();
    if (!operation.date.isValid()) {
        operation.date = QDateTime::currentDateTime();
    }
    return operation;
}

QJsonObject rateToJson(const RateRecord &rate)
{
    QJsonObject object;
    object["bank"] = rate.bank;
    object["rate"] = rate.rate;
    object["description"] = rate.description;
    object["source"] = rate.source;
    object["fetched_at"] = rate.fetchedAt;
    return object;
}

RateRecord rateFromJson(const QJsonObject &object)
{
    RateRecord rate;
    rate.id = object.value("id").toInt(-1);
    rate.bank = object.value("bank").toString();
    rate.rate = object.value("rate").toDouble();
    rate.description = object.value("description").toString();
    rate.source = object.value("source").toString();
    rate.fetchedAt = object.value("fetched_at").toString();
    return rate;
}

QJsonObject currencyToJson(const CurrencyRate &rate)
{
    QJsonObject object;
    object["code"] = rate.code;
    object["name"] = rate.name;
    object["value"] = rate.value;
    object["source"] = rate.source;
    object["fetched_at"] = rate.fetchedAt;
    return object;
}

CurrencyRate currencyFromJson(const QJsonObject &object)
{
    CurrencyRate rate;
    rate.id = object.value("id").toInt(-1);
    rate.code = object.value("code").toString();
    rate.name = object.value("name").toString();
    rate.value = object.value("value").toDouble();
    rate.source = object.value("source").toString();
    rate.fetchedAt = object.value("fetched_at").toString();
    return rate;
}

} // namespace

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
{
}

void NetworkClient::setEndpoint(const QString &host, quint16 port)
{
    m_host = host.trimmed().isEmpty() ? QStringLiteral("127.0.0.1") : host.trimmed();
    m_port = port == 0 ? 45454 : port;
}

QString NetworkClient::host() const
{
    return m_host;
}

quint16 NetworkClient::port() const
{
    return m_port;
}

bool NetworkClient::testConnection(QString *error, int timeoutMs) const
{
    QJsonObject payload;
    payload["command"] = "ping";
    const QJsonObject response = request(payload, error, timeoutMs);
    return response.value("ok").toBool(false);
}

bool NetworkClient::login(const QString &login, const QString &password, QString *error)
{
    QJsonObject payload;
    payload["command"] = "login";
    payload["login"] = login;
    payload["password"] = password;

    const QJsonObject response = request(payload, error);
    if (!response.value("ok").toBool(false)) {
        if (error && error->isEmpty()) {
            *error = response.value("error").toString("Не удалось войти.");
        }
        return false;
    }

    m_sessionToken = response.value("session_token").toString();
    m_userId = response.value("user_id").toInt(-1);
    m_login = login;
    return hasSession();
}

bool NetworkClient::registerUser(const QString &login, const QString &password, QString *error)
{
    QJsonObject payload;
    payload["command"] = "register";
    payload["login"] = login;
    payload["password"] = password;

    const QJsonObject response = request(payload, error);
    if (!response.value("ok").toBool(false)) {
        if (error && error->isEmpty()) {
            *error = response.value("error").toString("Регистрация не выполнена.");
        }
        return false;
    }
    return true;
}

QString NetworkClient::sessionToken() const
{
    return m_sessionToken;
}

int NetworkClient::userId() const
{
    return m_userId;
}

QString NetworkClient::loginName() const
{
    return m_login;
}

bool NetworkClient::hasSession() const
{
    return !m_sessionToken.isEmpty() && m_userId > 0;
}

void NetworkClient::clearSession()
{
    m_sessionToken.clear();
    m_login.clear();
    m_userId = -1;
}

double NetworkClient::getBalance(QString *error)
{
    const QJsonObject response = request(authenticatedPayload("getBalance"), error);
    return response.value("balance").toDouble(0.0);
}

bool NetworkClient::setBalance(double amount, QString *error)
{
    QJsonObject payload = authenticatedPayload("setBalance");
    payload["amount"] = amount;
    return request(payload, error).value("ok").toBool(false);
}

bool NetworkClient::addOperation(const FinanceOperation &operation, QString *error)
{
    QJsonObject payload = authenticatedPayload("addOperation");
    payload["operation"] = operationToJson(operation);
    return request(payload, error).value("ok").toBool(false);
}

QVector<FinanceOperation> NetworkClient::listOperations(QString *error)
{
    QVector<FinanceOperation> operations;
    const QJsonObject response = request(authenticatedPayload("listOperations"), error);
    const QJsonArray array = response.value("operations").toArray();
    for (const QJsonValue &value : array) {
        operations.append(operationFromJson(value.toObject()));
    }
    return operations;
}

bool NetworkClient::removeLastOperation(QString *error)
{
    return request(authenticatedPayload("removeLastOperation"), error).value("ok").toBool(false);
}

bool NetworkClient::clearOperations(QString *error)
{
    return request(authenticatedPayload("clearOperations"), error).value("ok").toBool(false);
}

QStringList NetworkClient::listCategories(QString *error)
{
    QStringList categories;
    const QJsonObject response = request(authenticatedPayload("listCategories"), error);
    const QJsonArray array = response.value("categories").toArray();
    for (const QJsonValue &value : array) {
        categories.append(value.toString());
    }
    return categories;
}

bool NetworkClient::addCategory(const QString &category, QString *error)
{
    QJsonObject payload = authenticatedPayload("addCategory");
    payload["category"] = category;
    return request(payload, error).value("ok").toBool(false);
}

QVector<RateRecord> NetworkClient::listCreditRates(QString *error)
{
    return listRates("listCreditRates", error);
}

QVector<RateRecord> NetworkClient::listDepositRates(QString *error)
{
    return listRates("listDepositRates", error);
}

QVector<CurrencyRate> NetworkClient::listCurrencyRates(QString *error)
{
    QVector<CurrencyRate> rates;
    const QJsonObject response = request(authenticatedPayload("listCurrencyRates"), error);
    const QJsonArray array = response.value("rates").toArray();
    for (const QJsonValue &value : array) {
        rates.append(currencyFromJson(value.toObject()));
    }
    return rates;
}

bool NetworkClient::saveCreditRates(const QVector<RateRecord> &rates, QString *error)
{
    return saveRates("saveCreditRates", rates, error);
}

bool NetworkClient::saveDepositRates(const QVector<RateRecord> &rates, QString *error)
{
    return saveRates("saveDepositRates", rates, error);
}

bool NetworkClient::saveCurrencyRates(const QVector<CurrencyRate> &rates, QString *error)
{
    QJsonObject payload = authenticatedPayload("saveCurrencyRates");
    QJsonArray array;
    for (const CurrencyRate &rate : rates) {
        array.append(currencyToJson(rate));
    }
    payload["rates"] = array;
    return request(payload, error).value("ok").toBool(false);
}

QJsonObject NetworkClient::request(const QJsonObject &payload, QString *error, int timeoutMs) const
{
    if (error) {
        error->clear();
    }

    QTcpSocket socket;
    socket.connectToHost(m_host, m_port);
    if (!socket.waitForConnected(timeoutMs)) {
        if (error) {
            *error = QString("Сервер %1:%2 недоступен: %3").arg(m_host).arg(m_port).arg(socket.errorString());
        }
        return {};
    }

    const QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact) + '\n';
    socket.write(data);
    if (!socket.waitForBytesWritten(timeoutMs)) {
        if (error) {
            *error = QString("Не удалось отправить запрос: %1").arg(socket.errorString());
        }
        return {};
    }

    QByteArray responseLine;
    while (!responseLine.contains('\n')) {
        if (!socket.waitForReadyRead(timeoutMs)) {
            if (error) {
                *error = QString("Сервер не ответил вовремя: %1").arg(socket.errorString());
            }
            return {};
        }
        responseLine += socket.readAll();
    }

    const int newlineIndex = responseLine.indexOf('\n');
    const QByteArray jsonLine = responseLine.left(newlineIndex);
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(jsonLine, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) {
            *error = QString("Сервер вернул некорректный JSON: %1").arg(parseError.errorString());
        }
        return {};
    }

    const QJsonObject response = document.object();
    if (!response.value("ok").toBool(false) && error) {
        *error = response.value("error").toString("Сервер вернул ошибку.");
    }
    return response;
}

QJsonObject NetworkClient::authenticatedPayload(const QString &command) const
{
    QJsonObject payload;
    payload["command"] = command;
    payload["session_token"] = m_sessionToken;
    return payload;
}

QVector<RateRecord> NetworkClient::listRates(const QString &command, QString *error)
{
    QVector<RateRecord> rates;
    const QJsonObject response = request(authenticatedPayload(command), error);
    const QJsonArray array = response.value("rates").toArray();
    for (const QJsonValue &value : array) {
        rates.append(rateFromJson(value.toObject()));
    }
    return rates;
}

bool NetworkClient::saveRates(const QString &command, const QVector<RateRecord> &rates, QString *error)
{
    QJsonObject payload = authenticatedPayload(command);
    QJsonArray array;
    for (const RateRecord &rate : rates) {
        array.append(rateToJson(rate));
    }
    payload["rates"] = array;
    return request(payload, error).value("ok").toBool(false);
}
