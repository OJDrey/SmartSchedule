#include "schedulehtml.h"
#include <QFile>
#include <QMap>
#include <QPair>
#include <QPdfWriter>
#include <QPageSize>
#include <QPageLayout>
#include <QPrinter>
#include <QTextDocument>

namespace {
QString escape(const QString &text) {
    return text.toHtmlEscaped().replace(kNewLine, "<br>");
}
}

QString ScheduleHtml::page(const QString &title, const QString &subtitle, const QList<BellItem> &bells,
                           const QList<ScheduleRecord> &records, ViewMode mode, bool breakBefore) {
    QString html = QString("<h2 align='center'%1>%2</h2>")
                       .arg(breakBefore ? QString(" style='page-break-before: always;'") : QString(), escape(title));
    if (!subtitle.isEmpty()) {
        html += QString("<p align='center'>%1</p>").arg(escape(subtitle));
    }

    QMap<QPair<int, int>, QList<ScheduleRecord>> cells;
    for (const auto &r : records) {
        cells[qMakePair(r.pairNumber, r.dayOfWeek)].append(r);
    }

    html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'><tr><th></th>";
    for (const auto &day : scheduleDayNames()) {
        html += "<th>" + escape(day) + "</th>";
    }
    html += "</tr>";

    for (const auto &bell : bells) {
        html += QString("<tr><td><b>%1 пара</b><br>%2 - %3</td>").arg(bell.pairNumber).arg(bell.startTime, bell.endTime);

        for (int day = 1; day <= 5; ++day) {
            const auto list = cells.value(qMakePair(bell.pairNumber, day));
            if (list.isEmpty()) {
                html += "<td></td>";
                continue;
            }

            QColor background = list.size() > 1 ? QColor("#E3F2FD") : QColor(list.first().colorHex);
            if (!background.isValid()) background = QColor("#4CAF50");
            const QString textColor = background.lightness() < 128 ? "#FFFFFF" : "#000000";

            QStringList parts;
            for (const auto &r : list) {
                parts << QString("<b>%1</b><br>%2").arg(escape(recordTitle(r)), escape(recordDetails(r, mode)));
            }

            html += QString("<td align='center' bgcolor='%1'><font color='%2'>%3</font></td>")
                        .arg(background.name(), textColor, parts.join("<hr>"));
        }
        html += "</tr>";
    }

    html += "</table>";
    return html;
}

QString ScheduleHtml::document(const QStringList &pages) {
    return pages.join(QString());
}

bool ScheduleHtml::savePdf(const QString &html, const QString &path) {
    {
        QPdfWriter writer(path);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageOrientation(QPageLayout::Landscape);
        writer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);

        QTextDocument doc;
        doc.setHtml(html);
        doc.print(&writer);
    }
    return QFile::exists(path);
}

void ScheduleHtml::print(const QString &html, QPrinter *printer) {
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(printer);
}
