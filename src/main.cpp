//    PartVault — simple inventory manager for electronic components
//    Copyright (C) 2026-...  Oleksandr Kolodkin <oleksandr.kolodkin@ukr.net>
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <QtLogging>
#include <QMessageLogContext>
#include <QApplication>
#include <QStyleFactory>
#include <QLocale>
#include <QLibraryInfo>
#include <QTranslator>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QDirIterator>
#include <QStandardPaths>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#include <cstdio>
#endif

#include "database.h"
#include "mainwindow.h"

QtMessageHandler originalMessageHandler = nullptr;
QFile logFile("partvault.log");
static QtMsgType g_logLevel = QtWarningMsg;

QString messageTypeToString(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:    return "DEBUG";
        case QtInfoMsg:     return "INFO";
        case QtWarningMsg:  return "WARNING";
        case QtCriticalMsg: return "CRITICAL";
        case QtFatalMsg:    return "FATAL";
        default:            return "UNKNOWN";
    }
}

static QtMsgType parseMsgType(const QString& s, bool& ok) {
    ok = true;
    const QString lower = s.toLower();
    if (lower == "debug")    return QtDebugMsg;
    if (lower == "info")     return QtInfoMsg;
    if (lower == "warning")  return QtWarningMsg;
    if (lower == "critical") return QtCriticalMsg;
    if (lower == "fatal")    return QtFatalMsg;
    ok = false;
    return QtDebugMsg;
}

bool openLogFile() {
    if (!logFile.isOpen())
        if (!logFile.open(QIODevice::Append | QIODevice::Text))
            return false;
    return true;
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {

    // Filter by configured log level
    if (type < g_logLevel) return;

    // Attempt to log to file; if it fails, we still want to call the original handler 
    // to ensure the message is visible on the console.
    if (openLogFile()) {

        // Format: [timestamp] [file:line] [type]: message
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

        // Ensure the message is flushed to disk immediately
        out.flush();
    }

    // Call the original message handler (e.g. to also log to console)
    if (originalMessageHandler) originalMessageHandler(type, context, msg);
}

int main(int argc, char *argv[]) {

#ifdef Q_OS_WIN
    // On Windows the app is built with the GUI subsystem (WIN32), so it has no
    // console by default.  Attach to the parent process console (e.g. PowerShell /
    // cmd) so that --help / --version output is actually visible.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* f = nullptr;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
    }
#endif

    // Install our custom message handler to log to a file, while preserving 
    // the original handler's behavior (e.g. logging to console).
    originalMessageHandler = qInstallMessageHandler(messageHandler);

    QApplication a(argc, argv);
    a.setApplicationName("PartVault");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("Oleksandr Kolodkin");
    a.setStyle(QStyleFactory::create("Fusion"));

    // ── Command-line parsing ──────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription("PartVault — inventory manager for electronic components");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption logLevelOption(
        "log-level",
        "Minimum log level to record (debug|info|warning|critical|fatal). Default: warning.",
        "level",
        "warning"
    );
    parser.addOption(logLevelOption);

    QCommandLineOption dummyOption(
        "dummy",
        "Populate the database with sample data on start."
    );
    parser.addOption(dummyOption);

    QCommandLineOption resetOption(
        "reset",
        "Drop and recreate the database on start."
    );
    parser.addOption(resetOption);

    // process() calls exit(0) for --help / --version, so the GUI is never started.
    parser.process(a);

    const bool wantDummy  = parser.isSet(dummyOption);
    const bool wantReset  = parser.isSet(resetOption);
    const QString logLevelStr = parser.value(logLevelOption);

    // Validate log level
    bool levelOk = false;
    g_logLevel = parseMsgType(logLevelStr, levelOk);
    if (!levelOk) {
        qCritical() << "Unknown log level:" << logLevelStr
                    << "(valid: debug, info, warning, critical, fatal)";
        return 1;
    }

    // If logging debug messages
    if (g_logLevel <= QtDebugMsg) {
        // Print some additional environment info to help 
        // with debugging issues related to missing translation files, etc.
        qDebug() << "Debug mode is enabled.";
        qDebug() << "Application locale:" << QLocale::system().bcp47Name();

        // List all available resources for debugging purposes.  
        // This can help identify issues with missing translation files, etc.
        qDebug() << "Available resources:";
        QDirIterator it(":", QDirIterator::Subdirectories);
        while (it.hasNext()) qDebug() << it.next();
    }

    // Load Qt's built-in translations for standard dialogs, etc.
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

    // Paths to search for the application's own translation files (e.g. for UI strings).
    const QStringList ApplicationTranslationPaths = {
        ":/i18n",
        qApp->applicationDirPath() + "/i18n/",
        qApp->applicationDirPath() + "/../i18n/",
        qApp->applicationDirPath() + "/../share/part-vault/i18n/",
        qApp->applicationDirPath() + "/translations/",
        qApp->applicationDirPath() + "/../translations/",
        qApp->applicationDirPath() + "/../share/part-vault/translations/"
    };

    // Load the application's own translations (e.g. for UI strings).
    QTranslator myTranslator;
    for (const auto &ApplicationTranslationPath : ApplicationTranslationPaths) {
        if (myTranslator.load(
            QLocale(),
            QLatin1String("PartVault"),
            QLatin1String("_"),
            ApplicationTranslationPath
        )) {
            a.installTranslator(&myTranslator);
            qDebug() << "Application translator installed from" << ApplicationTranslationPath;
            break;
        } else {
            qDebug() << "Application translator not found in" << ApplicationTranslationPath;
        }
    }

    // Initialize the database manager and open the database connection.
    QString dataDir = QDir::homePath() + "/.partvault";

    // Ensure the data directory exists, creating it if necessary.
    if (!QDir().mkpath(dataDir)) {
        qCritical() << "Failed to create writable data directory:" << dataDir;
        return 1;
    }

    // The database file will be located inside the data directory.
    const QString dbPath = QDir(dataDir).filePath("parts.db");
    qDebug() << "Using database file:" << dbPath;

    DatabaseManager databaseManager(dbPath);

    // If --reset is specified, the database will be dropped and recreated.
    // This is useful for testing or recovering from a corrupted database.
    if (!databaseManager.openDatabase(wantReset))
        qFatal() << "Failed to open database.";

    // If --dummy is specified, populate the database with sample data.
    // This is useful for testing or demo purposes.
    if (wantDummy)
        if (!databaseManager.addDummyData())
            qCritical() << "Failed to load dummy data.";

    // Start the main window and restore the previous session (e.g. open tabs, etc.).
    MainWindow w(databaseManager);
    w.show();
    w.restoreSession();

    // Run the application event loop and log the exit code when it finishes.
    auto exitCode = a.exec();
    qDebug() << "Application exiting with code" << exitCode;
    return exitCode;
}
