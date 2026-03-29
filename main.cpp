#include "mainwindow.h"

#include <QtLogging>
#include <QMessageLogContext>
#include <QApplication>
#include <QStyleFactory>
#include <QLocale>
#include <QLibraryInfo>
#include <QTranslator>
#include <QDebug>

#ifndef NDEBUG
#include <QLocale>
#include <QDirIterator>
#endif

#include "database.h"

QtMessageHandler originalMessageHandler = nullptr;
QFile logFile("partvault.log");

QString messageTypeToString(QtMsgType type) {
    switch (type) {
        case QtDebugMsg: return "DEBUG";
        case QtInfoMsg: return "INFO";
        case QtWarningMsg: return "WARNING";
        case QtCriticalMsg: return "CRITICAL";
        case QtFatalMsg: return "FATAL";
        default: return "UNKNOWN";
    }
}

bool openLogFile() {
    if (!logFile.isOpen())
        if (!logFile.open(QIODevice::Append | QIODevice::Text))
            return false;

    return true;
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {

    // Attempt to open the log file if it's not already open
    if (openLogFile()) {

        // Format the log message
        QString logMessage = QString("[%1] [%2:%3] [%4]: %5").arg(
            QDateTime::currentDateTime().toString(Qt::ISODate),
            context.file ? context.file : "unknown",
            QString::number(context.line),
            messageTypeToString(type),
            msg
        );

        // Write the log message to the file
        QTextStream out(&logFile);
        out << logMessage << "\n";

        // Ensure the message is written to the file immediately
        out.flush();
    }

    // Also print the log message to the console by calling the original message handler
    if (originalMessageHandler) originalMessageHandler(type, context, msg);
}

int main(int argc, char *argv[])
{
    // Install the custom message handler and save the original one
    originalMessageHandler = qInstallMessageHandler(messageHandler);

    QApplication a(argc, argv);
    a.setApplicationName("PartVault");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("Oleksandr Kolodkin");
    a.setStyle(QStyleFactory::create("Fusion"));

#ifndef NDEBUG
    qDebug() << "Debug mode is enabled.";
    qDebug() << "Application locale: " << QLocale::system().bcp47Name();
    qDebug() << "Available resources:";
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) qDebug() << it.next();
#endif

    QTranslator qtTranslator;
    if (qtTranslator.load(
        QLocale(),
        QLatin1String("qt"),
        QLatin1String("_"),
        QLibraryInfo::path(QLibraryInfo::TranslationsPath)
    )) {
        a.installTranslator(&qtTranslator);
        qDebug() << "Qt translator installed.";
    } else {
        qDebug() << "Qt translator not found.";
    }

    const QStringList ApplicationTranslationPaths = {
        ":/i18n",
        qApp->applicationDirPath() + "/i18n/",
        qApp->applicationDirPath() + "/../i18n/",
        qApp->applicationDirPath() + "/../share/part-vault/i18n/",
        qApp->applicationDirPath() + "/translations/",
        qApp->applicationDirPath() + "/../translations/",
        qApp->applicationDirPath() + "/../share/part-vault/translations/"
    };

    QTranslator myTranslator;
    for (const auto &ApplicationTranslationPath : ApplicationTranslationPaths) {
        if (myTranslator.load(
            QLocale(),
            QLatin1String("PartVault"),
            QLatin1String("_"),
            ApplicationTranslationPath
        )) {
            a.installTranslator(&myTranslator);
            qDebug() << "Application translator installed from " << ApplicationTranslationPath << ".";
            break;
        } else {
            qDebug() << "Application translator not found in " << ApplicationTranslationPath << ".";
        }
    }

    DatabaseManager databaseManager("partvault.db");
    databaseManager.openDatabase();

    MainWindow w;
    w.setDatabaseManager(&databaseManager);
    w.show();
    w.restoreSession();

    auto exitCode = a.exec();

    qDebug() << "Application exiting with code" << exitCode << ".";
    logFile.close();

    return exitCode;
}
