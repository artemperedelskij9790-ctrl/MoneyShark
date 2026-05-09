#include "depositcalculator.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

DepositCalculator::DepositCalculator(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

DepositCalculator::~DepositCalculator() = default;

void DepositCalculator::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel(QStringLiteral("Калькулятор доходности вклада"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QGroupBox *inputGroup = new QGroupBox(QStringLiteral("Параметры вклада"), this);
    QGridLayout *inputLayout = new QGridLayout(inputGroup);
    inputLayout->setVerticalSpacing(12);

    m_amountEdit = new QLineEdit(QStringLiteral("100000"), inputGroup);
    m_rateEdit = new QLineEdit(QStringLiteral("12"), inputGroup);
    m_termCombo = new QComboBox(inputGroup);
    m_termCombo->addItem(QStringLiteral("3 месяца"), 3);
    m_termCombo->addItem(QStringLiteral("6 месяцев"), 6);
    m_termCombo->addItem(QStringLiteral("12 месяцев"), 12);
    m_termCombo->addItem(QStringLiteral("24 месяца"), 24);
    m_termCombo->addItem(QStringLiteral("36 месяцев"), 36);
    m_termCombo->setCurrentIndex(2);
    m_customTermEdit = new QLineEdit(QStringLiteral("12"), inputGroup);
    m_paymentCombo = new QComboBox(inputGroup);
    m_paymentCombo->addItem(QStringLiteral("Ежемесячно"), 1);
    m_paymentCombo->addItem(QStringLiteral("Ежеквартально"), 3);
    m_paymentCombo->addItem(QStringLiteral("В конце срока"), 0);
    m_capitalizationCheck = new QCheckBox(QStringLiteral("Капитализация процентов"), inputGroup);
    m_capitalizationCheck->setChecked(true);

    inputLayout->addWidget(new QLabel(QStringLiteral("Сумма вклада, руб.:")), 0, 0);
    inputLayout->addWidget(m_amountEdit, 0, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Годовая ставка, %:")), 1, 0);
    inputLayout->addWidget(m_rateEdit, 1, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Срок:")), 2, 0);
    inputLayout->addWidget(m_termCombo, 2, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Срок в месяцах:")), 3, 0);
    inputLayout->addWidget(m_customTermEdit, 3, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Выплата процентов:")), 4, 0);
    inputLayout->addWidget(m_paymentCombo, 4, 1);
    inputLayout->addWidget(m_capitalizationCheck, 5, 0, 1, 2);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    m_calculateButton = new QPushButton(QStringLiteral("Рассчитать доходность"), inputGroup);
    m_calculateButton->setProperty("success", true);
    m_clearButton = new QPushButton(QStringLiteral("Очистить"), inputGroup);
    m_clearButton->setProperty("secondary", true);
    buttonsLayout->addWidget(m_calculateButton);
    buttonsLayout->addWidget(m_clearButton);
    buttonsLayout->addStretch();
    inputLayout->addLayout(buttonsLayout, 6, 0, 1, 2);
    inputLayout->setColumnStretch(1, 1);
    mainLayout->addWidget(inputGroup);

    QGroupBox *resultGroup = new QGroupBox(QStringLiteral("Результат"), this);
    QGridLayout *resultLayout = new QGridLayout(resultGroup);
    m_monthlyProfitEdit = new QLineEdit(resultGroup);
    m_totalAmountEdit = new QLineEdit(resultGroup);
    m_totalProfitEdit = new QLineEdit(resultGroup);
    m_effectiveRateEdit = new QLineEdit(resultGroup);
    for (QLineEdit *edit : {m_monthlyProfitEdit, m_totalAmountEdit, m_totalProfitEdit, m_effectiveRateEdit}) {
        edit->setReadOnly(true);
    }
    resultLayout->addWidget(new QLabel(QStringLiteral("Средний доход в месяц:")), 0, 0);
    resultLayout->addWidget(m_monthlyProfitEdit, 0, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("Итоговая сумма:")), 1, 0);
    resultLayout->addWidget(m_totalAmountEdit, 1, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("Общая прибыль:")), 2, 0);
    resultLayout->addWidget(m_totalProfitEdit, 2, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("Эффективная ставка:")), 3, 0);
    resultLayout->addWidget(m_effectiveRateEdit, 3, 1);
    resultLayout->setColumnStretch(1, 1);
    mainLayout->addWidget(resultGroup);

    m_profitTable = new QTableWidget(this);
    m_profitTable->setColumnCount(5);
    m_profitTable->setHorizontalHeaderLabels({
        QStringLiteral("Период"),
        QStringLiteral("Сумма вклада"),
        QStringLiteral("Начислено"),
        QStringLiteral("Выплачено"),
        QStringLiteral("Итого")
    });
    m_profitTable->horizontalHeader()->setStretchLastSection(true);
    m_profitTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_profitTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_profitTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_profitTable, 1);

    m_backButton = new QPushButton(QStringLiteral("Назад в меню вкладов"), this);
    m_backButton->setProperty("secondary", true);
    mainLayout->addWidget(m_backButton);

    connect(m_calculateButton, &QPushButton::clicked, this, &DepositCalculator::calculateDeposit);
    connect(m_clearButton, &QPushButton::clicked, this, &DepositCalculator::clearForm);
    connect(m_backButton, &QPushButton::clicked, this, &DepositCalculator::backToMenu);
    connect(m_customTermEdit, &QLineEdit::editingFinished, this, &DepositCalculator::onTermEditingFinished);
    connect(m_termCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_customTermEdit->setText(QString::number(m_termCombo->itemData(index).toInt()));
    });
    connect(m_capitalizationCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_paymentCombo->setEnabled(!checked);
    });
    m_paymentCombo->setEnabled(false);
}

void DepositCalculator::calculateDeposit()
{
    bool amountOk = false;
    bool rateOk = false;
    bool monthsOk = false;
    QString amountText = m_amountEdit->text();
    QString rateText = m_rateEdit->text();
    amountText.replace(',', '.');
    rateText.replace(',', '.');
    const double amount = amountText.toDouble(&amountOk);
    const double rate = rateText.toDouble(&rateOk);
    const int months = m_customTermEdit->text().toInt(&monthsOk);

    if (!amountOk || amount <= 0.0) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Введите корректную сумму вклада."));
        return;
    }
    if (!rateOk || rate < 0.0) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Введите корректную процентную ставку."));
        return;
    }
    if (!monthsOk || !isValidMonthValue(months)) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Введите срок от 1 до 600 месяцев."));
        return;
    }

    m_currentAmount = amount;
    m_currentRate = rate;
    m_currentMonths = months;
    m_currentCapitalization = m_capitalizationCheck->isChecked();

    const double finalAmount = calculateFinalAmount(amount, rate, months, m_currentCapitalization);
    const double monthlyProfit = calculateMonthlyRate(amount, rate, months, m_currentCapitalization);
    const double totalProfit = calculateTotalProfit(amount, rate, months, m_currentCapitalization);
    const double effectiveRate = calculateEffectiveRate(rate, months, m_currentCapitalization);

    m_monthlyProfitEdit->setText(QString("%1 руб.").arg(monthlyProfit, 0, 'f', 2));
    m_totalAmountEdit->setText(QString("%1 руб.").arg(finalAmount, 0, 'f', 2));
    m_totalProfitEdit->setText(QString("%1 руб.").arg(totalProfit, 0, 'f', 2));
    m_effectiveRateEdit->setText(QString("%1 % годовых").arg(effectiveRate, 0, 'f', 2));
    updateTable();
}

void DepositCalculator::clearForm()
{
    m_amountEdit->setText(QStringLiteral("100000"));
    m_rateEdit->setText(QStringLiteral("12"));
    m_termCombo->setCurrentIndex(2);
    m_customTermEdit->setText(QStringLiteral("12"));
    m_paymentCombo->setCurrentIndex(0);
    m_capitalizationCheck->setChecked(true);
    m_monthlyProfitEdit->clear();
    m_totalAmountEdit->clear();
    m_totalProfitEdit->clear();
    m_effectiveRateEdit->clear();
    m_profitTable->setRowCount(0);
    m_currentAmount = 0.0;
    m_currentRate = 0.0;
    m_currentMonths = 12;
    m_currentCapitalization = true;
}

void DepositCalculator::onTermEditingFinished()
{
    bool ok = false;
    const int months = m_customTermEdit->text().toInt(&ok);
    if (!ok || !isValidMonthValue(months)) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Введите срок от 1 до 600 месяцев."));
        m_customTermEdit->setText(QString::number(m_currentMonths > 0 ? m_currentMonths : 12));
        return;
    }

    const int index = m_termCombo->findData(months);
    if (index >= 0) {
        m_termCombo->setCurrentIndex(index);
    }
}

void DepositCalculator::updateTable()
{
    if (m_currentAmount <= 0.0 || m_currentMonths <= 0) {
        return;
    }

    const int displayPeriods = std::min(24, m_currentMonths);
    m_profitTable->setRowCount(displayPeriods);

    const double monthlyRate = m_currentRate / 100.0 / 12.0;
    const int paymentFrequency = m_paymentCombo->currentData().toInt();
    double currentAmount = m_currentAmount;
    double paidOut = 0.0;

    for (int month = 1; month <= displayPeriods; ++month) {
        const double interest = currentAmount * monthlyRate;
        double paidNow = 0.0;
        if (m_currentCapitalization) {
            currentAmount += interest;
        } else {
            const bool endOfTerm = paymentFrequency == 0 && month == m_currentMonths;
            const bool scheduled = paymentFrequency > 0 && month % paymentFrequency == 0;
            if (scheduled || endOfTerm) {
                paidNow = interest;
                paidOut += paidNow;
            }
        }

        const double total = m_currentCapitalization ? currentAmount : m_currentAmount + paidOut;
        m_profitTable->setItem(month - 1, 0, new QTableWidgetItem(QString("Месяц %1").arg(month)));
        m_profitTable->setItem(month - 1, 1, new QTableWidgetItem(QString("%1").arg(m_currentCapitalization ? currentAmount : m_currentAmount, 0, 'f', 2)));
        m_profitTable->setItem(month - 1, 2, new QTableWidgetItem(QString("%1").arg(interest, 0, 'f', 2)));
        m_profitTable->setItem(month - 1, 3, new QTableWidgetItem(QString("%1").arg(paidNow, 0, 'f', 2)));
        m_profitTable->setItem(month - 1, 4, new QTableWidgetItem(QString("%1").arg(total, 0, 'f', 2)));
    }
    m_profitTable->resizeColumnsToContents();
}

double DepositCalculator::calculateFinalAmount(double amount, double rate, int months, bool withCapitalization)
{
    if (amount <= 0.0 || months <= 0) {
        return 0.0;
    }
    if (rate <= 0.0) {
        return amount;
    }
    if (withCapitalization) {
        const double monthlyRate = rate / 100.0 / 12.0;
        return amount * std::pow(1.0 + monthlyRate, months);
    }
    return amount * (1.0 + rate / 100.0 * months / 12.0);
}

double DepositCalculator::calculateMonthlyRate(double amount, double rate, int months, bool withCapitalization)
{
    if (months <= 0) {
        return 0.0;
    }
    return calculateTotalProfit(amount, rate, months, withCapitalization) / months;
}

double DepositCalculator::calculateTotalProfit(double amount, double rate, int months, bool withCapitalization)
{
    return calculateFinalAmount(amount, rate, months, withCapitalization) - amount;
}

double DepositCalculator::calculateEffectiveRate(double rate, int months, bool withCapitalization)
{
    if (!withCapitalization || months <= 0) {
        return rate;
    }
    const double monthlyRate = rate / 100.0 / 12.0;
    return (std::pow(1.0 + monthlyRate, 12.0) - 1.0) * 100.0;
}

bool DepositCalculator::isValidMonthValue(int months)
{
    return months >= 1 && months <= 600;
}
