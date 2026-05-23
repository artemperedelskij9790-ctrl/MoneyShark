#ifndef MONEYSHARKSERVER_H
#define MONEYSHARKSERVER_H

#include <QHash>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QTcpServer>

class MoneySharkServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit MoneySharkServer(QObject *parent = nullptr);
    bool start(quint16 port, const QString &databasePath);

private slots:
    void acceptClient();

private:
    bool initDatabase(const QString &databasePath);
    bool createTables();
    bool execSql(const QString &sql);
    int userCount();
    int findUserId(const QString &login);
    int ensureAdminUser();
    bool createUser(const QString &login, const QString &password, QString *error = nullptr);
    bool checkPassword(const QString &login, const QString &password, int *userId = nullptr);
    void ensureDefaultCategories(int userId);

    QJsonObject handleRequest(const QJsonObject &request);
    int authenticatedUserId(const QJsonObject &request) const;
    QJsonObject makeError(const QString &error) const;
    QJsonObject makeOk() const;
    QString makeToken(int userId);

    QJsonObject getBalance(int userId);
    QJsonObject setBalance(int userId, const QJsonObject &request);
    QJsonObject addOperation(int userId, const QJsonObject &request);
    QJsonObject listOperations(int userId);
    QJsonObject removeLastOperation(int userId);
    QJsonObject clearOperations(int userId);
    QJsonObject listCategories(int userId);
    QJsonObject addCategory(int userId, const QJsonObject &request);
    QJsonObject listRates(const QString &tableName, const QString &orderBy);
    QJsonObject saveRates(const QString &tableName, const QJsonObject &request);
    QJsonObject listCurrencyRates();
    QJsonObject saveCurrencyRates(const QJsonObject &request);

    QSqlDatabase m_database;
    QHash<QString, int> m_sessions;
};

#endif // MONEYSHARKSERVER_H
