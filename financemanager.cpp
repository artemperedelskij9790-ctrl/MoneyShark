#include "financemanager.h"
#include "database.h"

FinanceManager& FinanceManager::instance()
{
    static FinanceManager manager;
    return manager;
}

FinanceManager::FinanceManager()
{
}

void FinanceManager::loadFromDatabase()
{
    m_operations = DatabaseManager::instance().loadAllOperations();
    m_categories = DatabaseManager::instance().loadAllCategories();
}

void FinanceManager::addOperation(const FinanceOperation &operation)
{
    if (DatabaseManager::instance().saveOperation(operation)) {
        m_operations.append(operation);
    }
}

bool FinanceManager::removeLastOperation(FinanceOperation *removedOperation)
{
    if (m_operations.isEmpty()) {
        return false;
    }

    const FinanceOperation last = m_operations.last();
    if (!DatabaseManager::instance().removeLastOperation()) {
        return false;
    }

    m_operations.removeLast();
    if (removedOperation) {
        *removedOperation = last;
    }
    return true;
}

void FinanceManager::clearAllOperations()
{
    if (DatabaseManager::instance().clearAllOperations()) {
        m_operations.clear();
    }
}

double FinanceManager::getTotalIncome() const
{
    double total = 0.0;
    for (const FinanceOperation &operation : m_operations) {
        if (operation.type == FinanceOperation::Income) {
            total += operation.amount;
        }
    }
    return total;
}

double FinanceManager::getTotalExpense() const
{
    double total = 0.0;
    for (const FinanceOperation &operation : m_operations) {
        if (operation.type == FinanceOperation::Expense) {
            total += operation.amount;
        }
    }
    return total;
}

double FinanceManager::getOperationsBalance() const
{
    return getTotalIncome() - getTotalExpense();
}

const QVector<FinanceOperation>& FinanceManager::getOperations() const
{
    return m_operations;
}

QStringList FinanceManager::getCategories()
{
    if (m_categories.isEmpty()) {
        m_categories = DatabaseManager::instance().loadAllCategories();
    }
    return m_categories;
}

void FinanceManager::addCategory(const QString &category)
{
    const QString cleanCategory = category.trimmed();
    if (cleanCategory.isEmpty()) {
        return;
    }

    if (DatabaseManager::instance().saveCategory(cleanCategory) && !m_categories.contains(cleanCategory)) {
        m_categories.append(cleanCategory);
        m_categories.sort(Qt::CaseInsensitive);
    }
}

void FinanceManager::resetCache()
{
    m_operations.clear();
    m_categories.clear();
}
