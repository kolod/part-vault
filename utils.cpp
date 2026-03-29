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

#include "utils.h"

QString stripSqlComments(const QString& sql) {
    const QStringList lines = sql.split(QLatin1Char('\n'));
    QString result;
    for (const QString& line : lines) {
        QString stripped;
        bool inString = false;
        for (int i = 0; i < line.size(); ++i) {
            const QChar c = line[i];
            if (inString) {
                stripped += c;
                // A pair of '' inside a string is an escaped quote, not end-of-string
                if (c == QLatin1Char('\'')) {
                    if (i + 1 < line.size() && line[i + 1] == QLatin1Char('\''))
                        stripped += line[++i];   // consume the second ' and stay in string
                    else
                        inString = false;
                }
            } else {
                if (c == QLatin1Char('\'')) {
                    inString = true;
                    stripped += c;
                } else if (c == QLatin1Char('-') && i + 1 < line.size() && line[i + 1] == QLatin1Char('-')) {
                    break;   // rest of line is a comment
                } else {
                    stripped += c;
                }
            }
        }
        stripped = stripped.trimmed();
        if (!stripped.isEmpty())
            result += stripped + "\n";
    }
    return result;
}
