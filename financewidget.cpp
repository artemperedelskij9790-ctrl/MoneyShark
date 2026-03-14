#include "financewidget.h"
#include "database.h"

#include <QDateTime>
#include <QGridLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>

FinanceWidget::FinanceWidget(QWidget *parent, double *mainBalance)
    : QWidget(parent)
    , m_manager(FinanceManager::instance())
    , m_mainBalance(mainBalance)
{
    setupUI();
    m_manager.loadFromDatabase();
    refreshUI();
}

void FinanceWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel(QStringLiteral("Ручная запись финансов"));
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QWidget *inputPanel = new QWidget(this);
    inputPanel->setObjectName(QStringLiteral("panel"));
    QGridLayout *inputLayout = new QGridLayout(inputPanel);
    inputLayout->setContentsMargins(16, 16, 16, 16);
    inputLayout->setVerticalSpacing(12);
    inputLayout->setHorizontalSpacing(14);

    m_typeCombo = new QComboBox(inputPanel);
    m_typeCombo->addItem(QStringLiteral("Доход"), FinanceOperation::Income);
    m_typeCombo->addItem(QStringLiteral("Расход"), FinanceOperation::Expense);

    m_categoryCombo = new QComboBox(inputPanel);
    m_categoryCombo->setEditable(true);

    m_amountEdit = new QLineEdit(inputPanel);
    m_amountEdit->setPlaceholderText(QStringLiteral("Введите сумму"));

    m_noteEdit = new QTextEdit(inputPanel);
    m_noteEdit->setPlaceholderText(QStringLiteral("Примечание"));
    m_noteEdit->setMaximumHeight(70);

    inputLayout->addWidget(new QLabel(QStringLiteral("Тип:")), 0, 0);
    inputLayout->addWidget(m_typeCombo, 0, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Категория:")), 1, 0);
    inputLayout->addWidget(m_categoryCombo, 1, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Сумма, руб.:")), 2, 0);
    inputLayout->addWidget(m_amountEdit, 2, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Примечание:")), 3, 0);
    inputLayout->addWidget(m_noteEdit, 3, 1);

    QHBoxLayout *inputButtonsLayout = new QHBoxLayout();
    m_addButton = new QPushButton(QStringLiteral("Добавить операцию"), inputPanel);
    m_addButton->setProperty("success", true);
    m_addCategoryButton = new QPushButton(QStringLiteral("Новая категория"), inputPanel);
    m_addCategoryButton->setProperty("secondary", true);
    inputButtonsLayout->addWidget(m_addButton);
    inputButtonsLayout->addWidget(m_addCategoryButton);
    inputButtonsLayout->addStretch();
    inputLayout->addLayout(inputButtonsLayout, 4, 0, 1, 2);
    inputLayout->setColumnStretch(1, 1);
    mainLayout->addWidget(inputPanel);

    QWidget *statsPanel = new QWidget(this);
    statsPanel->setObjectName(QStringLiteral("panel"));
    QHBoxLayout *statsLayout = new QHBoxLayout(statsPanel);
    statsLayout->setContentsMargins(16, 12, 16, 12);
    m_totalIncomeLabel = new QLabel(statsPanel);
    m_totalExpenseLabel = new QLabel(statsPanel);
    m_balanceLabel = new QLabel(statsPanel);
    statsLayout->addWidget(m_totalIncomeLabel);
    statsLayout->addWidget(m_totalExpenseLabel);
    statsLayout->addWidget(m_balanceLabel);
    statsLayout->addStretch();
    mainLayout->addWidget(statsPanel);

    m_transactionTable = new QTableWidget(this);
    m_transactionTable->setColumnCount(5);
    m_transactionTable->setHorizontalHeaderLabels({
        QStringLiteral("Дата"),
        QStringLiteral("Тип"),
        QStringLiteral("Категория"),
        QStringLiteral("Сумма"),
        QStringLiteral("Примечание")
    });
    m_transactionTable->horizontalHeader()->setStretchLastSection(true);
    m_transactionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_transactionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_transactionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_transactionTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_transactionTable, 1);

    QHBoxLayout *controlLayout = new QHBoxLayout();
    m_removeLastButton = new QPushButton(QStringLiteral("Удалить последнюю операцию"), this);
    m_removeLastButton->setProperty("secondary", true);
    m_clearAllButton = new QPushButton(QStringLiteral("Очистить операции"), this);
    m_clearAllButton->setProperty("danger", true);
    m_backButton = new QPushButton(QStringLiteral("Назад в главное меню"), this);
    m_backButton->setProperty("secondary", true);
    controlLayout->addWidget(m_removeLastButton);
    controlLayout->addWidget(m_clearAllButton);
    controlLayout->addStretch();
    controlLayout->addWidget(m_backButton);
    mainLayout->addLayout(controlLayout);

    connect(m_addButton, &QPushButton::clicked, this, &FinanceWidget::addTransaction);
    connect(m_removeLastButton, &QPushButton::clicked, this, &FinanceWidget::removeLastTransaction);
    connect(m_clearAllButton, &QPushButton::clicked, this, &FinanceWidget::clearAllTransactions);
    connect(m_addCategoryButton, &QPushButton::clicked, this, &FinanceWidget::addNewCategory);
    connect(m_backButton, &QPushButton::clicked, this, &FinanceWidget::backToMenu);
}

void FinanceWidget::refreshUI()
{
    updateStatistics();
    updateTransactionTable();

    const QString currentCategory = m_categoryCombo->currentText();
    m_categoryCombo->clear();
    m_categoryCombo->addItems(m_manager.getCategories());
    if (!currentCategory.isEmpty()) {
        m_categoryCombo->setCurrentText(currentCategory);
    }
}

void FinanceWidget::updateStatistics()
{
    const double totalIncome = m_manager.getTotalIncome();
    const double totalExpense = m_manager.getTotalExpense();
    const double operationsBalance = m_manager.getOperationsBalance();
    const double realBalance = m_mainBalance ? *m_mainBalance : DatabaseManager::instance().loadBalance();

    m_totalIncomeLabel->setText(QString("Доходы: %1 руб.").arg(totalIncome, 0, 'f', 2));
    m_totalExpenseLabel->setText(QString("Расходы: %1 руб.").arg(totalExpense, 0, 'f', 2));
    m_balanceLabel->setText(QString("Баланс: %1 руб. (операции: %2 руб.)")
                                .arg(realBalance, 0, 'f', 2)
                                .arg(operationsBalance, 0, 'f', 2));
}

void FinanceWidget::updateTransactionTable()
{
    const QVector<FinanceOperation> &operations = m_manager.getOperations();
    m_transactionTable->setRowCount(operations.size());

    for (int sourceIndex = operations.size() - 1; sourceIndex >= 0; --sourceIndex) {
        const int row = operations.size() - 1 - sourceIndex;
        const FinanceOperation &operation = operations[sourceIndex];

        m_transactionTable->setItem(row, 0, new QTableWidgetItem(operation.date.toString(QStringLiteral("dd.MM.yyyy HH:mm"))));
        m_transactionTable->setItem(row, 1, new QTableWidgetItem(operation.type == FinanceOperation::Income ? QStringLiteral("Доход") : QStringLiteral("Расход")));
        m_transactionTable->setItem(row, 2, new QTableWidgetItem(operation.category));
        m_transactionTable->setItem(row, 3, new QTableWidgetItem(QString("%1 руб.").arg(operation.amount, 0, 'f', 2)));
        m_transactionTable->setItem(row, 4, new QTableWidgetItem(operation.note));
    }
    m_transactionTable->resizeColumnsToContents();
}

void FinanceWidget::addTransaction()
{
    bool ok = false;
    QString amountText = m_amountEdit->text();
    amountText.replace(',', '.');
    const double amount = amountText.toDouble(&ok);
    if (!ok || amount <= 0.0) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Введите корректную сумму."));
        return;
    }

    const QString category = m_categoryCombo->currentText().trimmed();
    if (category.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Выберите или введите категорию."));
        return;
    }

    if (!m_manager.getCategories().contains(category)) {
        m_manager.addCategory(category);
    }

    FinanceOperation operation;
    operation.userId = DatabaseManager::instance().currentUserId();
    operation.type = static_cast<FinanceOperation::Type>(m_typeCombo->currentData().toInt());
    operation.category = category;
    operation.amount = amount;
    operation.date = QDateTime::currentDateTime();
    operation.note = m_noteEdit->toPlainText().trimmed();

    const int beforeCount = m_manager.getOperations().size();
    m_manager.addOperation(operation);
    if (m_manager.getOperations().size() == beforeCount) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось сохранить операцию."));
        return;
    }

    applyOperationToBalance(operation);
    m_amountEdit->clear();
    m_noteEdit->clear();
    refreshUI();
    QMessageBox::information(this, QStringLiteral("Готово"), QStringLiteral("Операция сохранена."));
}

void FinanceWidget::removeLastTransaction()
{
    if (m_manager.getOperations().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Операции"), QStringLiteral("Нет операций для удаления."));
        return;
    }

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("Подтверждение"),
        QStringLiteral("Удалить последнюю операцию?"),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    FinanceOperation removedOperation;
    if (m_manager.removeLastOperation(&removedOperation)) {
        applyOperationToBalance(removedOperation, true);
        refreshUI();
    }
}

void FinanceWidget::clearAllTransactions()
{
    if (m_manager.getOperations().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Операции"), QStringLiteral("Нет операций для очистки."));
        return;
    }

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("Подтверждение"),
        QStringLiteral("Удалить все операции текущего пользователя? Баланс останется без изменений."),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_manager.clearAllOperations();
        refreshUI();
    }
}

void FinanceWidget::addNewCategory()
{
    bool ok = false;
    const QString category = QInputDialog::getText(
        this,
        QStringLiteral("Новая категория"),
        QStringLiteral("Введите название категории:"),
        QLineEdit::Normal,
        QString(),
        &ok).trimmed();

    if (ok && !category.isEmpty()) {
        m_manager.addCategory(category);
        refreshUI();
    }
}

void FinanceWidget::applyOperationToBalance(const FinanceOperation &operation, bool reverse)
{
    if (!m_mainBalance) {
        return;
    }

    double delta = operation.type == FinanceOperation::Income ? operation.amount : -operation.amount;
    if (reverse) {
        delta = -delta;
    }
    *m_mainBalance += delta;
    DatabaseManager::instance().saveBalance(*m_mainBalance);
    emit balanceUpdated(*m_mainBalance);
}
