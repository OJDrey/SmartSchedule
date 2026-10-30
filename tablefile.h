#ifndef TABLEFILE_H
#define TABLEFILE_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QColor>
#include "csvutils.h"
#include "xlsxdocument.h"
#include "xlsxformat.h"
#include "xlsxcellrange.h"

namespace TableFile {

struct Sheet {
    QString name;
    QList<QStringList> rows;
};

inline bool isXlsx(const QString &path) {
    return path.endsWith(".xlsx", Qt::CaseInsensitive);
}

inline bool hasKnownSuffix(const QString &path) {
    return isXlsx(path) || path.endsWith(".csv", Qt::CaseInsensitive);
}

inline QList<QStringList> readCurrentSheet(QXlsx::Document &doc) {
    const QXlsx::CellRange range = doc.dimension();
    if (!range.isValid()) return {};

    QList<QStringList> rows;
    for (int r = range.firstRow(); r <= range.lastRow(); ++r) {
        QStringList row;
        bool empty = true;
        for (int c = range.firstColumn(); c <= range.lastColumn(); ++c) {
            const QString value = doc.read(r, c).toString().trimmed();
            if (!value.isEmpty()) empty = false;
            row << value;
        }
        if (!empty) rows.append(row);
    }
    return rows;
}

inline QList<QStringList> read(const QString &path, QString &error) {
    if (!isXlsx(path)) return Csv::read(path, error);

    QXlsx::Document doc(path);
    if (!doc.load()) {
        error = "Не вдалося відкрити файл Excel.";
        return {};
    }
    return readCurrentSheet(doc);
}

inline QList<QStringList> readSheet(const QString &path, const QString &sheetName, QString &error) {
    if (!isXlsx(path)) return Csv::read(path, error);

    QXlsx::Document doc(path);
    if (!doc.load()) {
        error = "Не вдалося відкрити файл Excel.";
        return {};
    }
    if (!doc.sheetNames().contains(sheetName)) {
        error = QString("Аркуш «%1» не знайдено. Імпортуйте файл, експортований цією програмою.").arg(sheetName);
        return {};
    }
    doc.selectSheet(sheetName);
    return readCurrentSheet(doc);
}

inline void writeRows(QXlsx::Document &doc, const QList<QStringList> &rows) {
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setBorderStyle(QXlsx::Format::BorderThin);
    headerFormat.setFillPattern(QXlsx::Format::PatternSolid);
    headerFormat.setPatternBackgroundColor(QColor("#BBDEFB"));

    QList<int> widths;
    for (int r = 0; r < rows.size(); ++r) {
        for (int c = 0; c < rows[r].size(); ++c) {
            const QString &text = rows[r][c];

            bool isNumber = false;
            const int number = text.toInt(&isNumber);
            const QVariant value = (isNumber && QString::number(number) == text) ? QVariant(number) : QVariant(text);

            doc.write(r + 1, c + 1, value, r == 0 ? headerFormat : QXlsx::Format());

            while (widths.size() <= c) widths << 10;
            widths[c] = qBound(10, qMax(widths[c], static_cast<int>(text.size()) + 2), 60);
        }
    }

    for (int c = 0; c < widths.size(); ++c) {
        doc.setColumnWidth(c + 1, widths[c]);
    }
}

inline bool writeSheets(const QString &path, const QList<Sheet> &sheets, QString &error) {
    if (sheets.isEmpty()) {
        error = "Немає даних для запису.";
        return false;
    }
    if (!isXlsx(path)) return Csv::write(path, sheets.first().rows, error);

    QXlsx::Document doc;
    const QString defaultSheet = doc.sheetNames().value(0);
    for (const auto &sheet : sheets) {
        doc.addSheet(sheet.name);
        writeRows(doc, sheet.rows);
    }
    doc.deleteSheet(defaultSheet);
    doc.selectSheet(sheets.first().name);

    if (!doc.saveAs(path)) {
        error = "не вдалося записати файл Excel.";
        return false;
    }
    return true;
}

inline bool write(const QString &path, const QList<QStringList> &rows, QString &error) {
    return writeSheets(path, {Sheet{"Аркуш1", rows}}, error);
}

}

#endif // TABLEFILE_H
