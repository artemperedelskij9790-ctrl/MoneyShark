#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

private slots:
    void login();
    void registerUser();
    void openConnectionSettings();
    void refreshModeLabel();

private:
    QLineEdit *m_loginEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLabel *m_modeLabel = nullptr;
};

#endif // LOGINDIALOG_H
