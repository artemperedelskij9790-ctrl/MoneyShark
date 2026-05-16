#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include "models.h"
#include "networkclient.h"

#include <QSqlDatabase>
#include <QStringList>

class DatabaseManager
{
public:
    enum class Mode {
        Local,
        Server
    };

    static DatabaseManager& instance();

    bool initDatabase(const QString &databasePath = QString());
    void closeDatabase();

    bool useLocalMode();
    bool useServerMode(const QString &host, quint16 port, QString *error = nullptr);
    Mode mode() const;
    bool isServerMode() const;
    QString connectionDescription() const;

    bool loginUser(const QString &login, const QString &password, QString *error = nullptr);
    bool registerUser(const QString &login, const QString &password, QString *error = nullptr);
    int userCount() const;
    int currentUserId() const;
    QString currentLogin() const;
    QString sessionToken() const;
    bool hasAuthenticatedUser() const;
    void clearCurrentUser();

    bool saveOperation(const FinanceOperation &operation);
    QVector<FinanceOperation> loadAllOperations();
    bool removeLastOperation();
    bool clearAllOperations();

    bool saveBalance(double balance);
    double loadBalance();

    bool saveCategory(const QString &category);
    QStringList loadAllCategories();
    bool clearAllCategories();

    bool saveCreditRates(const QVector<RateRecord> &rates);
    bool saveDepositRates(const QVector<RateRecord> &rates);
    bool saveCurrencyRates(const QVector<CurrencyRate> &rates);
    QVector<RateRecord> loadCreditRates();
    QVector<RateRecord> loadDepositRates();
    QVector<CurrencyRate> loadCurrencyRates();

    static QString hashPassword(const QString &password, const QString &salt);
    static QString makeSalt();

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool createAndMigrateTables();
    bool createCoreTables();
    bool tableExists(const QString &tableName) const;
    QStringList tableColumns(const QString &tableName) const;
    bool execSql(const QString &sql) const;
    int ensureDefaultAdmin();
    int findUserId(const QString &login) const;
    bool createUser(const QString &login, const QString &password, QString *error = nullptr);
    bool ensureDefaultCategories(int userId);

    bool saveRatesToTable(const QString &tableName, const QVector<RateRecord> &rates);
    QVector<RateRecord> loadRatesFromTable(const QString &tableName, const QString &orderBy);

    QSqlDatabase m_database;
    QString m_connectionName = QStringLiteral("moneyshark_local");
    Mode m_mode = Mode::Local;
    NetworkClient m_client;
    int m_currentUserId = -1;
    QString m_currentLogin;
};

#endif // DATABASEMANAGER_H
