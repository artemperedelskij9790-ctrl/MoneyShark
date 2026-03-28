#include "statisticswidget.h"

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QHBoxLayout>
#include <QLegend>
#include <QMap>
#include <QPainter>
#include <QPieSeries>
#include <QPieSlice>
#include <QValueAxis>
#include <QVBoxLayout>

StatisticsWidget::StatisticsWidget(QWidget *parent)
    : QWidget(parent)
    , m_manager(FinanceManager::instance())
{
    setupUI();
    refreshStats();
}

void StatisticsWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel(QStringLiteral("Статистика доходов и расходов"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

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

    m_tabWidget = new QTabWidget(this);
    m_incomeChartView = new QChartView(this);
    m_expenseChartView = new QChartView(this);
    m_barChartView = new QChartView(this);
    m_incomeChartView->setRenderHint(QPainter::Antialiasing);
    m_expenseChartView->setRenderHint(QPainter::Antialiasing);
    m_barChartView->setRenderHint(QPainter::Antialiasing);

    QWidget *incomeTab = new QWidget(this);
    QVBoxLayout *incomeLayout = new QVBoxLayout(incomeTab);
    incomeLayout->addWidget(m_incomeChartView);
    m_tabWidget->addTab(incomeTab, QStringLiteral("Доходы"));

    QWidget *expenseTab = new QWidget(this);
    QVBoxLayout *expenseLayout = new QVBoxLayout(expenseTab);
    expenseLayout->addWidget(m_expenseChartView);
    m_tabWidget->addTab(expenseTab, QStringLiteral("Расходы"));

    QWidget *comparisonTab = new QWidget(this);
    QVBoxLayout *comparisonLayout = new QVBoxLayout(comparisonTab);
    comparisonLayout->addWidget(m_barChartView);
    m_tabWidget->addTab(comparisonTab, QStringLiteral("Сравнение"));
    mainLayout->addWidget(m_tabWidget, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_refreshButton = new QPushButton(QStringLiteral("Обновить"), this);
    m_refreshButton->setProperty("secondary", true);
    m_backButton = new QPushButton(QStringLiteral("Назад в главное меню"), this);
    m_backButton->setProperty("secondary", true);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_backButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_refreshButton, &QPushButton::clicked, this, &StatisticsWidget::refreshStats);
    connect(m_backButton, &QPushButton::clicked, this, &StatisticsWidget::backToMenu);
}

void StatisticsWidget::refreshStats()
{
    m_manager.loadFromDatabase();
    updateStatistics();
    createIncomeChart();
    createExpenseChart();
    createComparisonChart();
}

void StatisticsWidget::updateStatistics()
{
    const double income = m_manager.getTotalIncome();
    const double expense = m_manager.getTotalExpense();
    const double balance = m_manager.getOperationsBalance();

    m_totalIncomeLabel->setText(QString("Доходы: %1 руб.").arg(income, 0, 'f', 2));
    m_totalExpenseLabel->setText(QString("Расходы: %1 руб.").arg(expense, 0, 'f', 2));
    m_balanceLabel->setText(QString("Итог по операциям: %1 руб.").arg(balance, 0, 'f', 2));
}

void StatisticsWidget::createIncomeChart()
{
    QMap<QString, double> totals;
    double totalIncome = 0.0;
    for (const FinanceOperation &operation : m_manager.getOperations()) {
        if (operation.type == FinanceOperation::Income) {
            totals[operation.category] += operation.amount;
            totalIncome += operation.amount;
        }
    }

    if (totals.isEmpty()) {
        m_incomeChartView->setChart(makeEmptyChart(QStringLiteral("Доходы по категориям")));
        return;
    }

    QPieSeries *series = new QPieSeries();
    for (auto it = totals.cbegin(); it != totals.cend(); ++it) {
        QPieSlice *slice = series->append(it.key(), it.value());
        slice->setLabelVisible(true);
        slice->setLabel(QString("%1: %2%").arg(it.key()).arg((it.value() / totalIncome) * 100.0, 0, 'f', 1));
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QStringLiteral("Доходы по категориям"));
    chart->setBackgroundBrush(QBrush(QColor("#0d253c")));
    chart->setTitleBrush(QBrush(QColor("#edf6ff")));
    chart->legend()->setLabelColor(QColor("#edf6ff"));
    chart->legend()->setAlignment(Qt::AlignBottom);
    m_incomeChartView->setChart(chart);
}

void StatisticsWidget::createExpenseChart()
{
    QMap<QString, double> totals;
    double totalExpense = 0.0;
    for (const FinanceOperation &operation : m_manager.getOperations()) {
        if (operation.type == FinanceOperation::Expense) {
            totals[operation.category] += operation.amount;
            totalExpense += operation.amount;
        }
    }

    if (totals.isEmpty()) {
        m_expenseChartView->setChart(makeEmptyChart(QStringLiteral("Расходы по категориям")));
        return;
    }

    QPieSeries *series = new QPieSeries();
    for (auto it = totals.cbegin(); it != totals.cend(); ++it) {
        QPieSlice *slice = series->append(it.key(), it.value());
        slice->setLabelVisible(true);
        slice->setLabel(QString("%1: %2%").arg(it.key()).arg((it.value() / totalExpense) * 100.0, 0, 'f', 1));
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QStringLiteral("Расходы по категориям"));
    chart->setBackgroundBrush(QBrush(QColor("#0d253c")));
    chart->setTitleBrush(QBrush(QColor("#edf6ff")));
    chart->legend()->setLabelColor(QColor("#edf6ff"));
    chart->legend()->setAlignment(Qt::AlignBottom);
    m_expenseChartView->setChart(chart);
}

void StatisticsWidget::createComparisonChart()
{
    QBarSet *incomeSet = new QBarSet(QStringLiteral("Доходы"));
    QBarSet *expenseSet = new QBarSet(QStringLiteral("Расходы"));
    *incomeSet << m_manager.getTotalIncome();
    *expenseSet << m_manager.getTotalExpense();
    incomeSet->setColor(QColor("#25a16e"));
    expenseSet->setColor(QColor("#bd4053"));

    QBarSeries *series = new QBarSeries();
    series->append(incomeSet);
    series->append(expenseSet);
    series->setLabelsVisible(true);
    series->setLabelsFormat(QStringLiteral("@value руб."));

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QStringLiteral("Сравнение доходов и расходов"));
    chart->setBackgroundBrush(QBrush(QColor("#0d253c")));
    chart->setTitleBrush(QBrush(QColor("#edf6ff")));
    chart->legend()->setLabelColor(QColor("#edf6ff"));
    chart->legend()->setAlignment(Qt::AlignBottom);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(QStringList{QStringLiteral("Текущий пользователь")});
    axisX->setLabelsColor(QColor("#edf6ff"));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat(QStringLiteral("%.0f"));
    axisY->setLabelsColor(QColor("#edf6ff"));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    m_barChartView->setChart(chart);
}

QChart *StatisticsWidget::makeEmptyChart(const QString &title) const
{
    QPieSeries *series = new QPieSeries();
    QPieSlice *slice = series->append(QStringLiteral("Нет данных"), 1.0);
    slice->setLabelVisible(true);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(title);
    chart->setBackgroundBrush(QBrush(QColor("#0d253c")));
    chart->setTitleBrush(QBrush(QColor("#edf6ff")));
    chart->legend()->setLabelColor(QColor("#edf6ff"));
    chart->legend()->setAlignment(Qt::AlignBottom);
    return chart;
}
