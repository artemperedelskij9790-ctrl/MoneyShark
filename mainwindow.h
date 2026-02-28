#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ratesprovider.h"

#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>

class CreditCalculator;
class DepositCalculator;
class FinanceWidget;
class StatisticsWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_pushButton_Finansy_clicked();
    void on_pushButton_Kredity_clicked();
    void on_pushButton_Vklady_clicked();
    void on_backToMain_1_clicked();
    void on_backToMain_2_clicked();
    void on_backToMain_3_clicked();
    void on_addMoney_clicked();
    void on_subtractMoney_clicked();
    void on_setCustomMoney_clicked();
    void openFinanceWidget();
    void openStatisticsWidget();
    void openCreditCalculator();
    void openDepositCalculator();
    void showCurrencyRates();
    void showCreditRates();
    void showDepositRates();
    void showCreditBanks();
    void showDepositBanks();
    void openConnectionSettings();

private:
    void setupMenuBar();
    void setupStackedWidget();
    QWidget *createMainPage();
    QWidget *createSubMenu(const QString &title, const QList<QPushButton*> &buttons, const QString &backButtonObjectName);
    QPushButton *makeMenuButton(const QString &text);
    void updateBalanceDisplay();
    void setMainTitle();
    void showRatesTable(const QString &title, const QVector<RateRecord> &rates, const QString &message);
    void showCurrencyTable(const QVector<CurrencyRate> &rates, const QString &message);
    void showBankList(const QString &title, const QVector<RateRecord> &rates, const QString &message);
    void reloadRatesSilently();

    QStackedWidget *m_stackedWidget = nullptr;
    QWidget *m_mainPage = nullptr;
    QWidget *m_subMenuFinance = nullptr;
    QWidget *m_subMenuCredits = nullptr;
    QWidget *m_subMenuDeposits = nullptr;
    FinanceWidget *m_financeWidget = nullptr;
    StatisticsWidget *m_statisticsWidget = nullptr;
    CreditCalculator *m_creditCalculatorWidget = nullptr;
    DepositCalculator *m_depositCalculatorWidget = nullptr;
    QLabel *m_balanceLabel = nullptr;
    QLabel *m_userLabel = nullptr;
    double m_currentBalance = 0.0;
    RatesProvider m_ratesProvider;
};

#endif // MAINWINDOW_H
