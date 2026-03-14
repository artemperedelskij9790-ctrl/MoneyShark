#ifndef FINANCEMANAGER_H
#define FINANCEMANAGER_H

#include "models.h"

#include <QStringList>
#include <QVector>

class FinanceManager
{
public:
    static FinanceManager& instance();

    void loadFromDatabase();
    void addOperation(const FinanceOperation &operation);
    bool removeLastOperation(FinanceOperation *removedOperation = nullptr);
    void clearAllOperations();

    double getTotalIncome() const;
    double getTotalExpense() const;
    double getOperationsBalance() const;
    const QVector<FinanceOperation>& getOperations() const;

    QStringList getCategories();
    void addCategory(const QString &category);
    void resetCache();

private:
    FinanceManager();
    FinanceManager(const FinanceManager&) = delete;
    FinanceManager& operator=(const FinanceManager&) = delete;

    QVector<FinanceOperation> m_operations;
    QStringList m_categories;
};

#endif // FINANCEMANAGER_H
