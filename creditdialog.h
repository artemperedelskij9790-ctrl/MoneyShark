#ifndef CREDITDIALOG_H
#define CREDITDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>

class CreditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreditDialog(QWidget *parent = nullptr);
    ~CreditDialog();

private slots:
    void loadCreditData();
    void calculatePayment();

private:
    void setupUI();
    QTableWidget *m_creditTable;
    QPushButton *m_refreshButton;
    QPushButton *m_calculateButton;
    QPushButton *m_closeButton;
    QLineEdit *m_amountEdit;
    QLineEdit *m_termEdit;
    QComboBox *m_bankCombo;
    QLabel *m_paymentLabel;
};

#endif // CREDITDIALOG_H