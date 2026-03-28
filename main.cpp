#include "mainwindow.h"

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

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("PartVault");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("Oleksandr Kolodkin");
    a.setStyle(QStyleFactory::create("Fusion"));

#ifndef NDEBUG
    qDebug() << "Debug mode is enabled.";
    qDebug() << "Application locale: " << QLocale::system().bcp47Name() << "\n";

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

    MainWindow w;
    w.show();
    w.restoreSession();

    return a.exec();
}
