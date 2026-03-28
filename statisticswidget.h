#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include "financemanager.h"

#include <QChart>
#include <QChartView>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QWidget>

class StatisticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsWidget(QWidget *parent = nullptr);

signals:
    void backToMenu();

private slots:
    void refreshStats();

private:
    void setupUI();
    void updateStatistics();
    void createIncomeChart();
    void createExpenseChart();
    void createComparisonChart();
    QChart *makeEmptyChart(const QString &title) const;

    FinanceManager &m_manager;
    QTabWidget *m_tabWidget = nullptr;
    QChartView *m_incomeChartView = nullptr;
    QChartView *m_expenseChartView = nullptr;
    QChartView *m_barChartView = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_backButton = nullptr;
    QLabel *m_totalIncomeLabel = nullptr;
    QLabel *m_totalExpenseLabel = nullptr;
    QLabel *m_balanceLabel = nullptr;
};

#endif // STATISTICSWIDGET_H
