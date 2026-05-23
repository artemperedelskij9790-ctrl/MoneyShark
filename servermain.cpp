#include "moneysharkserver.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStringList>

#include <cstdio>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

void consoleMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    const QString line = message + QStringLiteral("\r\n");

#ifdef Q_OS_WIN
    const HANDLE handle = (type == QtInfoMsg || type == QtDebugMsg)
        ? GetStdHandle(STD_OUTPUT_HANDLE)
        : GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode = 0;
    if (handle != INVALID_HANDLE_VALUE && handle != nullptr && GetConsoleMode(handle, &mode)) {
        DWORD written = 0;
        WriteConsoleW(handle, reinterpret_cast<const wchar_t *>(line.utf16()), static_cast<DWORD>(line.size()), &written, nullptr);
        return;
    }
#endif

    FILE *stream = (type == QtInfoMsg || type == QtDebugMsg) ? stdout : stderr;
    const QByteArray utf8 = line.toUtf8();
    fwrite(utf8.constData(), 1, static_cast<size_t>(utf8.size()), stream);
    fflush(stream);
}

} // namespace

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    qInstallMessageHandler(consoleMessageHandler);

    QCoreApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("MoneySharkServer"));
    application.setOrganizationName(QStringLiteral("MoneyShark"));

    quint16 port = 45454;
    QString databasePath = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("moneyshark_server.db"));

    const QStringList args = application.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QStringLiteral("--port") && i + 1 < args.size()) {
            port = static_cast<quint16>(args[++i].toUShort());
        } else if (args[i] == QStringLiteral("--db") && i + 1 < args.size()) {
            databasePath = args[++i];
        }
    }

    MoneySharkServer server;
    if (!server.start(port, databasePath)) {
        return 1;
    }

    return application.exec();
}
