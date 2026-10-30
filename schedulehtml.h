#ifndef SCHEDULEHTML_H
#define SCHEDULEHTML_H

#include <QString>
#include <QList>
#include <QStringList>
#include "databasemanager.h"
#include "schedulefilter.h"

class QPrinter;

class ScheduleHtml {
public:
    static QString page(const QString &title, const QString &subtitle, const QList<BellItem> &bells,
                        const QList<ScheduleRecord> &records, ViewMode mode, bool breakBefore = false);
    static QString document(const QStringList &pages);
    static bool savePdf(const QString &html, const QString &path);
    static void print(const QString &html, QPrinter *printer);
};

#endif // SCHEDULEHTML_H
