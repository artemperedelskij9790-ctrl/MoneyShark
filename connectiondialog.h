#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = nullptr);

    QString host() const;
    quint16 port() const;

private slots:
    void testConnection();
    void accept() override;

private:
    QLineEdit *m_hostEdit = nullptr;
    QSpinBox *m_portSpin = nullptr;
};

#endif // CONNECTIONDIALOG_H
