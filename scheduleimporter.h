#ifndef SCHEDULEIMPORTER_H
#define SCHEDULEIMPORTER_H

#include <QList>
#include <QString>
#include <QStringList>
#include "databasemanager.h"

struct ImportRow {
    int sourceRow = 0;
    ScheduleRecord record;
    QString groupName;
    QString subjectName;
    QString teacherName;
    QString roomName;
    QStringList missing;
    QString error;
};

struct ImportReport {
    int total = 0;
    int imported = 0;
    QStringList errors;
};

class ScheduleImporter {
public:
    static QStringList header();
    static QList<QStringList> toRows(const QList<ScheduleRecord> &records);
    static QList<ImportRow> parse(const QList<QStringList> &rows);
    static void resolveReferences(QList<ImportRow> &rows);
    static ImportReport run(QList<ImportRow> &rows, bool replaceGroups, bool createMissing);
};

#endif // SCHEDULEIMPORTER_H
