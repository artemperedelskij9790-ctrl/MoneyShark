#include "connectiondialog.h"
#include "networkclient.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Настройки подключения"));
    setMinimumWidth(420);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    QSettings settings(QStringLiteral("MoneyShark"), QStringLiteral("MoneyShark"));
    m_hostEdit = new QLineEdit(settings.value(QStringLiteral("server/host"), QStringLiteral("127.0.0.1")).toString(), this);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(settings.value(QStringLiteral("server/port"), 45454).toInt());

    formLayout->addRow(QStringLiteral("IP или имя хоста:"), m_hostEdit);
    formLayout->addRow(QStringLiteral("Порт:"), m_portSpin);
    mainLayout->addLayout(formLayout);

    QPushButton *testButton = new QPushButton(QStringLiteral("Проверить подключение"), this);
    testButton->setProperty("secondary", true);
    mainLayout->addWidget(testButton);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Сохранить"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Отмена"));
    mainLayout->addWidget(buttonBox);

    connect(testButton, &QPushButton::clicked, this, &ConnectionDialog::testConnection);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConnectionDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConnectionDialog::reject);
}

QString ConnectionDialog::host() const
{
    return m_hostEdit->text().trimmed();
}

quint16 ConnectionDialog::port() const
{
    return static_cast<quint16>(m_portSpin->value());
}

void ConnectionDialog::testConnection()
{
    NetworkClient client;
    client.setEndpoint(host(), port());
    QString error;
    if (client.testConnection(&error)) {
        QMessageBox::information(this, QStringLiteral("Подключение"), QStringLiteral("Сервер доступен."));
    } else {
        QMessageBox::warning(this, QStringLiteral("Подключение"), error);
    }
}

void ConnectionDialog::accept()
{
    if (host().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), QStringLiteral("Введите IP или имя хоста."));
        return;
    }

    QSettings settings(QStringLiteral("MoneyShark"), QStringLiteral("MoneyShark"));
    settings.setValue(QStringLiteral("server/host"), host());
    settings.setValue(QStringLiteral("server/port"), port());
    QDialog::accept();
}
