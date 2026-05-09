#ifndef DEPOSITCALCULATOR_H
#define DEPOSITCALCULATOR_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QCheckBox>

class DepositCalculator : public QWidget
{
    Q_OBJECT

public:
    explicit DepositCalculator(QWidget *parent = nullptr);
    ~DepositCalculator();

signals:
    void backToMenu();

private slots:
    void calculateDeposit();
    void clearForm();
    void onTermEditingFinished();

private:
    void setupUI();
    void updateTable();
    double calculateFinalAmount(double amount, double rate, int months, bool withCapitalization);
    double calculateMonthlyRate(double amount, double rate, int months, bool withCapitalization);
    double calculateTotalProfit(double amount, double rate, int months, bool withCapitalization);
    double calculateEffectiveRate(double rate, int months, bool withCapitalization);
    bool isValidMonthValue(int months);

    QLineEdit *m_amountEdit;
    QLineEdit *m_rateEdit;
    QComboBox *m_termCombo;
    QLineEdit *m_customTermEdit;
    QComboBox *m_paymentCombo;
    QCheckBox *m_capitalizationCheck;
    QLineEdit *m_monthlyProfitEdit;
    QLineEdit *m_totalAmountEdit;
    QLineEdit *m_totalProfitEdit;
    QLineEdit *m_effectiveRateEdit;
    QTableWidget *m_profitTable;
    QPushButton *m_calculateButton;
    QPushButton *m_clearButton;
    QPushButton *m_backButton;
    double m_currentAmount;
    double m_currentRate;
    int m_currentMonths;
    bool m_currentCapitalization;
};

#endif // DEPOSITCALCULATOR_H