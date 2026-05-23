#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include "models.h"

#include <QJsonObject>
#include <QObject>
#include <QStringList>

class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);

    void setEndpoint(const QString &host, quint16 port);
    QString host() const;
    quint16 port() const;
    bool testConnection(QString *error = nullptr, int timeoutMs = 1500) const;

    bool login(const QString &login, const QString &password, QString *error = nullptr);
    bool registerUser(const QString &login, const QString &password, QString *error = nullptr);

    QString sessionToken() const;
    int userId() const;
    QString loginName() const;
    bool hasSession() const;
    void clearSession();

    double getBalance(QString *error = nullptr);
    bool setBalance(double amount, QString *error = nullptr);
    bool addOperation(const FinanceOperation &operation, QString *error = nullptr);
    QVector<FinanceOperation> listOperations(QString *error = nullptr);
    bool removeLastOperation(QString *error = nullptr);
    bool clearOperations(QString *error = nullptr);
    QStringList listCategories(QString *error = nullptr);
    bool addCategory(const QString &category, QString *error = nullptr);

    QVector<RateRecord> listCreditRates(QString *error = nullptr);
    QVector<RateRecord> listDepositRates(QString *error = nullptr);
    QVector<CurrencyRate> listCurrencyRates(QString *error = nullptr);
    bool saveCreditRates(const QVector<RateRecord> &rates, QString *error = nullptr);
    bool saveDepositRates(const QVector<RateRecord> &rates, QString *error = nullptr);
    bool saveCurrencyRates(const QVector<CurrencyRate> &rates, QString *error = nullptr);

private:
    QJsonObject request(const QJsonObject &payload, QString *error = nullptr, int timeoutMs = 3500) const;
    QJsonObject authenticatedPayload(const QString &command) const;
    QVector<RateRecord> listRates(const QString &command, QString *error = nullptr);
    bool saveRates(const QString &command, const QVector<RateRecord> &rates, QString *error = nullptr);

    QString m_host = QStringLiteral("127.0.0.1");
    quint16 m_port = 45454;
    QString m_sessionToken;
    QString m_login;
    int m_userId = -1;
};

#endif // NETWORKCLIENT_H
