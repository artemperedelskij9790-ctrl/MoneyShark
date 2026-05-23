#include "appstyle.h"
#include "connectiondialog.h"
#include "database.h"
#include "logindialog.h"
#include "logo.h"
#include "mainwindow.h"

#include <QApplication>
#include <QDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

namespace {

void configureStorageMode(QWidget *parent)
{
    while (true) {
        QSettings settings(QStringLiteral("MoneyShark"), QStringLiteral("MoneyShark"));
        const QString host = settings.value(QStringLiteral("server/host"), QStringLiteral("127.0.0.1")).toString();
        const quint16 port = static_cast<quint16>(settings.value(QStringLiteral("server/port"), 45454).toInt());
        QString error;
        if (DatabaseManager::instance().useServerMode(host, port, &error)) {
            return;
        }

        QMessageBox messageBox(parent);
        messageBox.setWindowTitle(QStringLiteral("Подключение к серверу"));
        messageBox.setText(QString("Сервер %1:%2 недоступен.\n%3").arg(host).arg(port).arg(error));
        QPushButton *offlineButton = messageBox.addButton(QStringLiteral("Работать автономно"), QMessageBox::AcceptRole);
        QPushButton *retryButton = messageBox.addButton(QStringLiteral("Повторить"), QMessageBox::ActionRole);
        QPushButton *settingsButton = messageBox.addButton(QStringLiteral("Настройки"), QMessageBox::ActionRole);
        messageBox.exec();

        if (messageBox.clickedButton() == offlineButton) {
            DatabaseManager::instance().useLocalMode();
            return;
        }
        if (messageBox.clickedButton() == settingsButton) {
            ConnectionDialog dialog(parent);
            dialog.exec();
            continue;
        }
        if (messageBox.clickedButton() == retryButton) {
            continue;
        }

        DatabaseManager::instance().useLocalMode();
        return;
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("MoneyShark"));
    application.setOrganizationName(QStringLiteral("MoneyShark"));
    application.setStyleSheet(moneySharkStyleSheet());
    application.setWindowIcon(moneySharkIcon());

    if (!DatabaseManager::instance().initDatabase()) {
        QMessageBox::critical(nullptr, QStringLiteral("База данных"), QStringLiteral("Не удалось открыть локальную базу данных."));
        return 1;
    }

    configureStorageMode(nullptr);

    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow window;
    window.show();
    return application.exec();
}
