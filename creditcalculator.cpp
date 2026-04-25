#include "creditcalculator.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

CreditCalculator::CreditCalculator(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

CreditCalculator::~CreditCalculator() = default;

void CreditCalculator::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel(QStringLiteral("Кредитный калькулятор"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QGroupBox *inputGroup = new QGroupBox(QStringLiteral("Параметры кредита"), this);
    QGridLayout *inputLayout = new QGridLayout(inputGroup);
    inputLayout->setVerticalSpacing(12);

    m_amountEdit = new QLineEdit(QStringLiteral("100000"), inputGroup);
    m_rateEdit = new QLineEdit(QStringLiteral("15"), inputGroup);
    m_termCombo = new QComboBox(inputGroup);
    m_termCombo->addItem(QStringLiteral("6 месяцев"), 6);
    m_termCombo->addItem(QStringLiteral("12 месяцев"), 12);
    m_termCombo->addItem(QStringLiteral("24 месяца"), 24);
    m_termCombo->addItem(QStringLiteral("36 месяцев"), 36);
    m_termCombo->addItem(QStringLiteral("60 месяцев"), 60);
    m_termCombo->addItem(QStringLiteral("120 месяцев"), 120);
    m_termCombo->setCurrentIndex(1);
    m_customTermEdit = new QLineEdit(QStringLiteral("12"), inputGroup);

    inputLayout->addWidget(new QLabel(QStringLiteral("Сумма кредита, руб.:")), 0, 0);
    inputLayout->addWidget(m_amountEdit, 0, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Годовая ставка, %:")), 1, 0);
    inputLayout->addWidget(m_rateEdit, 1, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Срок:")), 2, 0);
    inputLayout->addWidget(m_termCombo, 2, 1);
    inputLayout->addWidget(new QLabel(QStringLiteral("Срок в месяцах:")), 3, 0);
    inputLayout->addWidget(m_customTermEdit, 3, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    m_calculateButton = new QPushButton(QStringLiteral("Рассчитать платёж"), inputGroup);
    m_calculateButton->setProperty("success", true);
    m_clearButton = new QPushButton(QStringLiteral("Очистить"), inputGroup);
    m_clearButton->setProperty("secondary", true);
    buttonsLayout->addWidget(m_calculateButton);
    buttonsLayout->addWidget(m_clearButton);
    buttonsLayout->addStretch();
    inputLayout->addLayout(buttonsLayout, 4, 0, 1, 2);
    inputLayout->setColumnStretch(1, 1);
    mainLayout->addWidget(inputGroup);

    QGroupBox *resultGroup = new QGroupBox(QStringLiteral("Результат"), this);
    QGridLayout *resultLayout = new QGridLayout(resultGroup);
    m_monthlyPaymentEdit = new QLineEdit(resultGroup);
    m_totalPaymentEdit = new QLineEdit(resultGroup);
    m_overpaymentEdit = new QLineEdit(resultGroup);
    for (QLineEdit *edit : {m_monthlyPaymentEdit, m_totalPaymentEdit, m_overpaymentEdit}) {
        edit->setReadOnly(true);
    }
    resultLayout->addWidget(new QLabel(QStringLiteral("Ежемесячный платёж:")), 0, 0);
    resultLayout->addWidget(m_monthlyPaymentEdit, 0, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("Общая сумма выплат:")), 1, 0);
    resultLayout->addWidget(m_totalPaymentEdit, 1, 1);
    resultLayout->addWidget(new QLabel(QStringLiteral("Переплата:")), 2, 0);
    resultLayout->addWidget(m_overpaymentEdit, 2, 1);
    resultLayout->setColumnStretch(1, 1);
    mainLayout->addWidget(resultGroup);

    m_amortizationTable = new QTableWidget(this);
    m_amortizationTable->setColumnCount(5);
    m_amortizationTable->setHorizontalHeaderLabels({
        QStringLiteral("Месяц"),
        QStringLiteral("Платёж"),
        QStringLiteral("Основной долг"),
        QStringLiteral("Проценты"),
        QStringLiteral("Остаток")
    });
    m_amortizationTable->horizontalHeader()->setStretchLastSection(true);
    m_amortizationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_amortizationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_amortizationTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_amortizationTable, 1);

    m_backButton = new QPushButton(QStringLiteral("Назад в меню кредитов"), this);
    m_backButton->setProperty("secondary", true);
    mainLayout->addWidget(m_backButton);

    connect(m_calculateButton, &QPushButton::clicked, this, &CreditCalculator::calculateCredit);
    connect(m_clearButton, &QPushButton::clicked, this, &CreditCalculator::clearForm);
    connect(m_backButton, &QPushButton::clicked, this, &CreditCalculator::backToMenu);
    connect(m_customTermEdit, &QLineEdit::editingFinished, this, &CreditCalculator::onTermEditingFinished);
    connect(m_termCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_customTermEdit->setText(QString::number(m_termCombo->itemData(index).toInt()));
    });
}

void CreditCalculator::calculateCredit()
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
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Введите корректную сумму кредита."));
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

    const double monthlyPayment = calculateMonthlyPayment(amount, rate, months);
    const double totalPayment = calculateTotalPayment(amount, rate, months);
    const double overpayment = calculateOverpayment(amount, rate, months);

    m_monthlyPaymentEdit->setText(QString("%1 руб.").arg(monthlyPayment, 0, 'f', 2));
    m_totalPaymentEdit->setText(QString("%1 руб.").arg(totalPayment, 0, 'f', 2));
    m_overpaymentEdit->setText(QString("%1 руб.").arg(overpayment, 0, 'f', 2));
    updateTable();
}

void CreditCalculator::clearForm()
{
    m_amountEdit->setText(QStringLiteral("100000"));
    m_rateEdit->setText(QStringLiteral("15"));
    m_termCombo->setCurrentIndex(1);
    m_customTermEdit->setText(QStringLiteral("12"));
    m_monthlyPaymentEdit->clear();
    m_totalPaymentEdit->clear();
    m_overpaymentEdit->clear();
    m_amortizationTable->setRowCount(0);
    m_currentAmount = 0.0;
    m_currentRate = 0.0;
    m_currentMonths = 12;
}

void CreditCalculator::onTermEditingFinished()
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

void CreditCalculator::updateTable()
{
    if (m_currentAmount <= 0.0 || m_currentMonths <= 0) {
        return;
    }

    const int displayMonths = std::min(12, m_currentMonths);
    m_amortizationTable->setRowCount(displayMonths);

    const double monthlyRate = m_currentRate / 100.0 / 12.0;
    const double monthlyPayment = calculateMonthlyPayment(m_currentAmount, m_currentRate, m_currentMonths);
    double remainingDebt = m_currentAmount;

    for (int month = 1; month <= displayMonths; ++month) {
        const double interestPaid = remainingDebt * monthlyRate;
        double principalPaid = monthlyPayment - interestPaid;
        if (m_currentRate == 0.0) {
            principalPaid = monthlyPayment;
        }
        principalPaid = std::min(principalPaid, remainingDebt);
        remainingDebt = std::max(0.0, remainingDebt - principalPaid);

        m_amortizationTable->setItem(month - 1, 0, new QTableWidgetItem(QString::number(month)));
        m_amortizationTable->setItem(month - 1, 1, new QTableWidgetItem(QString("%1").arg(monthlyPayment, 0, 'f', 2)));
        m_amortizationTable->setItem(month - 1, 2, new QTableWidgetItem(QString("%1").arg(principalPaid, 0, 'f', 2)));
        m_amortizationTable->setItem(month - 1, 3, new QTableWidgetItem(QString("%1").arg(interestPaid, 0, 'f', 2)));
        m_amortizationTable->setItem(month - 1, 4, new QTableWidgetItem(QString("%1").arg(remainingDebt, 0, 'f', 2)));
    }
    m_amortizationTable->resizeColumnsToContents();
}

double CreditCalculator::calculateMonthlyPayment(double amount, double rate, int months)
{
    if (amount <= 0.0 || months <= 0) {
        return 0.0;
    }
    if (rate <= 0.0) {
        return amount / months;
    }
    const double monthlyRate = rate / 100.0 / 12.0;
    const double factor = std::pow(1.0 + monthlyRate, months);
    return amount * monthlyRate * factor / (factor - 1.0);
}

double CreditCalculator::calculateTotalPayment(double amount, double rate, int months)
{
    return calculateMonthlyPayment(amount, rate, months) * months;
}

double CreditCalculator::calculateOverpayment(double amount, double rate, int months)
{
    return calculateTotalPayment(amount, rate, months) - amount;
}

bool CreditCalculator::isValidMonthValue(int months)
{
    return months >= 1 && months <= 600;
}
