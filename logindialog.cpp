#include "logindialog.h"
#include "connectiondialog.h"
#include "database.h"
#include "financemanager.h"
#include "logo.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Money Shark - вход"));
    setMinimumWidth(440);
    setModal(true);
    setWindowIcon(moneySharkIcon());

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);

    QLabel *titleLabel = new QLabel(QStringLiteral("Money Shark"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    MoneySharkLogoWidget *logoWidget = new MoneySharkLogoWidget(this);
    logoWidget->setFixedHeight(155);
    mainLayout->addWidget(logoWidget);

    m_modeLabel = new QLabel(this);
    m_modeLabel->setWordWrap(true);
    mainLayout->addWidget(m_modeLabel);

    QFormLayout *formLayout = new QFormLayout();
    m_loginEdit = new QLineEdit(QStringLiteral("admin"), this);
    m_passwordEdit = new QLineEdit(QStringLiteral("admin123"), this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow(QStringLiteral("Логин:"), m_loginEdit);
    formLayout->addRow(QStringLiteral("Пароль:"), m_passwordEdit);
    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *loginButton = new QPushButton(QStringLiteral("Войти"), this);
    loginButton->setProperty("success", true);
    QPushButton *registerButton = new QPushButton(QStringLiteral("Зарегистрировать"), this);
    registerButton->setProperty("secondary", true);
    QPushButton *settingsButton = new QPushButton(QStringLiteral("Настройки подключения"), this);
    settingsButton->setProperty("secondary", true);
    buttonLayout->addWidget(loginButton);
    buttonLayout->addWidget(registerButton);
    buttonLayout->addWidget(settingsButton);
    mainLayout->addLayout(buttonLayout);

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::login);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::registerUser);
    connect(settingsButton, &QPushButton::clicked, this, &LoginDialog::openConnectionSettings);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::login);
    refreshModeLabel();
}

void LoginDialog::login()
{
    QString error;
    if (!DatabaseManager::instance().loginUser(m_loginEdit->text(), m_passwordEdit->text(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Вход"), error);
        return;
    }

    FinanceManager::instance().resetCache();
    FinanceManager::instance().loadFromDatabase();
    accept();
}

void LoginDialog::registerUser()
{
    QString error;
    if (!DatabaseManager::instance().registerUser(m_loginEdit->text(), m_passwordEdit->text(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Регистрация"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Регистрация"), QStringLiteral("Пользователь создан. Теперь можно войти."));
}

void LoginDialog::openConnectionSettings()
{
    ConnectionDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("Подключение"),
        QStringLiteral("Попробовать подключиться к серверу с новыми настройками?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QString error;
        if (!DatabaseManager::instance().useServerMode(dialog.host(), dialog.port(), &error)) {
            QMessageBox::warning(this, QStringLiteral("Подключение"), error + QStringLiteral("\nПриложение останется в автономном режиме."));
            DatabaseManager::instance().useLocalMode();
        }
    }
    refreshModeLabel();
}

void LoginDialog::refreshModeLabel()
{
    m_modeLabel->setText(QString("Режим работы: %1").arg(DatabaseManager::instance().connectionDescription()));
}
