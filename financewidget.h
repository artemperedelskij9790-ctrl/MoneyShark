#ifndef FINANCEWIDGET_H
#define FINANCEWIDGET_H

#include "financemanager.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QWidget>

class FinanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FinanceWidget(QWidget *parent = nullptr, double *mainBalance = nullptr);

signals:
    void backToMenu();
    void balanceUpdated(double balance);

private slots:
    void addTransaction();
    void removeLastTransaction();
    void clearAllTransactions();
    void addNewCategory();
    void refreshUI();

private:
    void setupUI();
    void updateStatistics();
    void updateTransactionTable();
    void applyOperationToBalance(const FinanceOperation &operation, bool reverse = false);

    FinanceManager &m_manager;
    double *m_mainBalance = nullptr;

    QComboBox *m_typeCombo = nullptr;
    QComboBox *m_categoryCombo = nullptr;
    QLineEdit *m_amountEdit = nullptr;
    QTextEdit *m_noteEdit = nullptr;
    QLabel *m_totalIncomeLabel = nullptr;
    QLabel *m_totalExpenseLabel = nullptr;
    QLabel *m_balanceLabel = nullptr;
    QTableWidget *m_transactionTable = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_addCategoryButton = nullptr;
    QPushButton *m_removeLastButton = nullptr;
    QPushButton *m_clearAllButton = nullptr;
    QPushButton *m_backButton = nullptr;
};

#endif // FINANCEWIDGET_H
