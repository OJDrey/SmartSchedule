#ifndef CSVUTILS_H
#define CSVUTILS_H

#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QStringList>
#include <QList>
#include "databasemanager.h"

namespace Csv {

inline constexpr QChar Quote('"');
inline constexpr QChar Separator(';');

inline QString escape(const QString &value) {
    const bool needsQuotes = value.contains(Separator) || value.contains(Quote) || value.contains(kNewLine);
    if (!needsQuotes) return value;

    QString escaped = value;
    escaped.replace(Quote, QString(Quote) + Quote);
    return QString(Quote) + escaped + Quote;
}

inline bool write(const QString &path, const QList<QStringList> &rows, QString &error) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        error = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out.setGenerateByteOrderMark(true);

    for (const auto &row : rows) {
        QStringList cells;
        for (const auto &cell : row) {
            cells << escape(cell);
        }
        out << cells.join(Separator) << kNewLine;
    }
    return true;
}

inline QList<QStringList> read(const QString &path, QString &error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = file.errorString();
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString text = in.readAll();
    if (text.startsWith(QChar(0xFEFF))) text.remove(0, 1);

    const int eol = text.indexOf(kNewLine);
    const QString firstLine = eol < 0 ? text : text.left(eol);
    const QChar separator = firstLine.count(QChar(';')) >= firstLine.count(QChar(',')) ? QChar(';') : QChar(',');

    QList<QStringList> rows;
    QStringList row;
    QString cell;
    bool quoted = false;

    auto endCell = [&] {
        row << cell;
        cell.clear();
    };
    auto endRow = [&] {
        endCell();
        if (!(row.size() == 1 && row.first().trimmed().isEmpty())) rows.append(row);
        row.clear();
    };

    for (int i = 0; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (quoted) {
            if (ch == Quote) {
                if (i + 1 < text.size() && text.at(i + 1) == Quote) {
                    cell += Quote;
                    ++i;
                } else {
                    quoted = false;
                }
            } else {
                cell += ch;
            }
        } else if (ch == Quote) {
            quoted = true;
        } else if (ch == separator) {
            endCell();
        } else if (ch == kNewLine) {
            endRow();
        } else if (ch != QChar(QChar::CarriageReturn)) {
            cell += ch;
        }
    }
    if (!cell.isEmpty() || !row.isEmpty()) endRow();

    return rows;
}

}

#endif // CSVUTILS_H