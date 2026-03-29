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

#include <QTest>
#include "../utils.h"

class TstStripSqlComments : public QObject
{
    Q_OBJECT

private slots:
    // A pure comment line is removed entirely
    void pureCommentLine() {
        const QString input  = "-- this is a comment\n";
        const QString result = stripSqlComments(input);
        QVERIFY(result.isEmpty());
    }

    // An empty line is removed
    void emptyLine() {
        QVERIFY(stripSqlComments("\n").isEmpty());
        QVERIFY(stripSqlComments("   \n").isEmpty());
    }

    // Code before an inline comment is kept; comment is stripped
    void inlineComment() {
        const QString input  = "    (52, 4);   -- LM2596-5               -> lm2596.pdf\n";
        const QString result = stripSqlComments(input);
        QCOMPARE(result, QString("(52, 4);\n"));
    }

    // A line with no comment passes through (trimmed)
    void noComment() {
        const QString input  = "SELECT * FROM parts;\n";
        const QString result = stripSqlComments(input);
        QCOMPARE(result, QString("SELECT * FROM parts;\n"));
    }

    // '--' inside a single-quoted string must NOT be treated as a comment
    void dashDashInsideString() {
        const QString input  = "INSERT INTO t VALUES ('hello -- world');\n";
        const QString result = stripSqlComments(input);
        QCOMPARE(result, QString("INSERT INTO t VALUES ('hello -- world');\n"));
    }

    // Escaped single-quote ('') inside a string must keep the parser in string mode
    void escapedQuoteInsideString() {
        const QString input  = "INSERT INTO t VALUES ('it''s fine -- not a comment');\n";
        const QString result = stripSqlComments(input);
        QCOMPARE(result, QString("INSERT INTO t VALUES ('it''s fine -- not a comment');\n"));
    }

    // Inline comment after a string value
    void inlineCommentAfterString() {
        const QString input  = "INSERT INTO t VALUES ('foo'); -- comment\n";
        const QString result = stripSqlComments(input);
        QCOMPARE(result, QString("INSERT INTO t VALUES ('foo');\n"));
    }

    // Multiple lines: mix of comments, blank lines and real SQL
    void multiLine() {
        const QString input =
            "-- header comment\n"
            "\n"
            "CREATE TABLE t (id INTEGER); -- inline\n"
            "INSERT INTO t VALUES (1);    -- row 1\n"
            "\n";

        const QString expected =
            "CREATE TABLE t (id INTEGER);\n"
            "INSERT INTO t VALUES (1);\n";

        QCOMPARE(stripSqlComments(input), expected);
    }

    // Input with no newline at the end
    void noTrailingNewline() {
        const QString input  = "SELECT 1; -- comment";
        const QString result = stripSqlComments(input);
        QCOMPARE(result, QString("SELECT 1;\n"));
    }

    // Completely empty input
    void emptyInput() {
        QVERIFY(stripSqlComments(QString()).isEmpty());
    }

    // Single hyphen (not --) must not be treated as a comment
    void singleHyphen() {
        const QString input  = "SELECT 1-1;\n";
        const QString result = stripSqlComments(input);
        QCOMPARE(result, QString("SELECT 1-1;\n"));
    }
};

QTEST_MAIN(TstStripSqlComments)
#include "tst_stripsqlcomments.moc"
