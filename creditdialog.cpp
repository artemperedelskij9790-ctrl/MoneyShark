#include "creditdialog.h"
#include "database.h"
#include "ratesprovider.h"

#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <cmath>

CreditDialog::CreditDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    loadCreditData();
}

CreditDialog::~CreditDialog() = default;

void CreditDialog::setupUI()
{
    setWindowTitle(QStringLiteral("Кредиты"));
    setMinimumSize(800, 560);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QLabel *titleLabel = new QLabel(QStringLiteral("Кредитные предложения"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    m_creditTable = new QTableWidget(this);
    m_creditTable->setColumnCount(4);
    m_creditTable->setHorizontalHeaderLabels({
        QStringLiteral("Банк"),
        QStringLiteral("Ставка, %"),
        QStringLiteral("Описание"),
        QStringLiteral("Дата обновления")
    });
    m_creditTable->horizontalHeader()->setStretchLastSection(true);
    m_creditTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_creditTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_creditTable, 1);

    QWidget *calcWidget = new QWidget(this);
    QFormLayout *formLayout = new QFormLayout(calcWidget);
    m_bankCombo = new QComboBox(calcWidget);
    m_amountEdit = new QLineEdit(calcWidget);
    m_amountEdit->setPlaceholderText(QStringLiteral("Сумма кредита"));
    m_termEdit = new QLineEdit(calcWidget);
    m_termEdit->setPlaceholderText(QStringLiteral("Срок в месяцах"));
    formLayout->addRow(QStringLiteral("Банк:"), m_bankCombo);
    formLayout->addRow(QStringLiteral("Сумма, руб.:"), m_amountEdit);
    formLayout->addRow(QStringLiteral("Срок, мес.:"), m_termEdit);
    mainLayout->addWidget(calcWidget);

    m_paymentLabel = new QLabel(this);
    m_paymentLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_paymentLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_refreshButton = new QPushButton(QStringLiteral("Обновить данные"), this);
    m_calculateButton = new QPushButton(QStringLiteral("Рассчитать платёж"), this);
    m_calculateButton->setProperty("success", true);
    m_closeButton = new QPushButton(QStringLiteral("Закрыть"), this);
    m_closeButton->setProperty("secondary", true);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_calculateButton);
    buttonLayout->addWidget(m_closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_refreshButton, &QPushButton::clicked, this, &CreditDialog::loadCreditData);
    connect(m_calculateButton, &QPushButton::clicked, this, &CreditDialog::calculatePayment);
    connect(m_closeButton, &QPushButton::clicked, this, &CreditDialog::accept);
}

void CreditDialog::loadCreditData()
{
    RatesProvider provider(this);
    QString message;
    const QVector<RateRecord> rates = provider.creditRatesWithFallback(&message);
    m_creditTable->setRowCount(rates.size());
    m_bankCombo->clear();

    for (int row = 0; row < rates.size(); ++row) {
        const RateRecord &rate = rates[row];
        m_creditTable->setItem(row, 0, new QTableWidgetItem(rate.bank));
        m_creditTable->setItem(row, 1, new QTableWidgetItem(QString::number(rate.rate, 'f', 2)));
        m_creditTable->setItem(row, 2, new QTableWidgetItem(rate.description));
        m_creditTable->setItem(row, 3, new QTableWidgetItem(rate.fetchedAt));
        m_bankCombo->addItem(rate.bank, rate.rate);
    }
    m_creditTable->resizeColumnsToContents();
    if (!message.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Данные"), message);
    }
}

void CreditDialog::calculatePayment()
{
    bool amountOk = false;
    bool monthsOk = false;
    QString amountText = m_amountEdit->text();
    amountText.replace(',', '.');
    const double amount = amountText.toDouble(&amountOk);
    const int months = m_termEdit->text().toInt(&monthsOk);
    const double rate = m_bankCombo->currentData().toDouble();

    if (!amountOk || amount <= 0.0 || !monthsOk || months <= 0 || rate <= 0.0) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Введите корректные сумму и срок."));
        return;
    }

    const double monthlyRate = rate / 100.0 / 12.0;
    const double factor = std::pow(1.0 + monthlyRate, months);
    const double payment = amount * monthlyRate * factor / (factor - 1.0);
    m_paymentLabel->setText(QString("Ежемесячный платёж: %1 руб.\nОбщая сумма выплат: %2 руб.")
                                .arg(payment, 0, 'f', 2)
                                .arg(payment * months, 0, 'f', 2));
}
