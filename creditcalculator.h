#ifndef CREDITCALCULATOR_H
#define CREDITCALCULATOR_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>

class CreditCalculator : public QWidget
{
    Q_OBJECT

public:
    explicit CreditCalculator(QWidget *parent = nullptr);
    ~CreditCalculator();

signals:
    void backToMenu();

private slots:
    void calculateCredit();
    void clearForm();
    void onTermEditingFinished();

private:
    void setupUI();
    void updateTable();
    double calculateMonthlyPayment(double amount, double rate, int months);
    double calculateTotalPayment(double amount, double rate, int months);
    double calculateOverpayment(double amount, double rate, int months);
    bool isValidMonthValue(int months);

    QLineEdit *m_amountEdit;
    QLineEdit *m_rateEdit;
    QComboBox *m_termCombo;
    QLineEdit *m_customTermEdit;
    QLineEdit *m_monthlyPaymentEdit;
    QLineEdit *m_totalPaymentEdit;
    QLineEdit *m_overpaymentEdit;
    QTableWidget *m_amortizationTable;
    QPushButton *m_calculateButton;
    QPushButton *m_clearButton;
    QPushButton *m_backButton;
    double m_currentAmount;
    double m_currentRate;
    int m_currentMonths;
};

#endif // CREDITCALCULATOR_H