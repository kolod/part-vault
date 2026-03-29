#include "database.h"
#include <QFile>
#include <QDebug>
#include <algorithm>

DatabaseManager::DatabaseManager(const QString& dbPath) : m_dbPath(dbPath) {
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(m_dbPath);
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::openDatabase() {
    // Check if the database file exists; if not, it will be created when we open it
    if (!m_database.open()) {
        qWarning() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }

    // Initialize the database schema if it's a new database
    if (!initializeDatabase()) {
        qWarning() << "Failed to initialize database";
        return false;
    }

    // Database opened and initialized successfully
    qDebug() << "Database opened successfully";
    return true;
}

void DatabaseManager::closeDatabase() {
    if (m_database.isOpen()) m_database.close();
}

QSqlDatabase& DatabaseManager::database() {
    return m_database;
}

bool DatabaseManager::initializeDatabase() {
    // Open the SQL initialization script from the resource system
    QFile file(":/sql/init.sql");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Failed to open init.sql resource";
        return false;
    }

    // Read the entire SQL script into a QString
    const QString sql = QString::fromUtf8(file.readAll()).trimmed();
    file.close();

    // Remove comments from the SQL script (lines starting with '--')
    const QStringList lines = sql.split(QLatin1Char('\n'));
    QString cleanedSql;
    for (const QString& line : lines) {
        const QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("--") || trimmedLine.isEmpty()) continue;
        cleanedSql += line + "\n"; // Preserve original formatting for non-comment lines
    }

    // Execute the SQL script to initialize the database schema.
    // QSqlQuery::exec() handles one statement at a time, so split on ';'.
    QSqlQuery query(m_database);
    const QStringList statements = cleanedSql.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString& statement : statements) {

        // Trim the statement to remove leading/trailing whitespace
        const QString trimmed = statement.trimmed();

        // Skip empty statements that may result from splitting
        if (trimmed.isEmpty()) continue;

        // Execute the SQL statement
        if (!query.exec(trimmed)) {
            qCritical() << "Failed to initialize database:" << query.lastError().text()
                        << "\nStatement:" << trimmed;
            return false;
        }
    }

    // Database initialized successfully
    qDebug() << "Database initialized successfully";
    return true;
}