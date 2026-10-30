#include "scheduleimporter.h"
#include "schedulefilter.h"
#include <QHash>
#include <QSet>

namespace {
QString normalize(const QString &text) {
    QString t = text.trimmed();
    t.replace(QChar(0x2019), QChar(0x27));
    t.replace(QChar(0x02BC), QChar(0x27));
    return t.toLower();
}

int parseDay(const QString &text) {
    const QString key = normalize(text);
    bool ok = false;
    const int number = key.toInt(&ok);
    if (ok) return (number >= 1 && number <= 5) ? number : 0;

    const auto &names = scheduleDayNames();
    for (int i = 0; i < names.size(); ++i) {
        if (normalize(names[i]) == key) return i + 1;
    }
    return 0;
}

int parseWeek(const QString &text) {
    const QString t = text.trimmed();
    if (t.isEmpty()) return 0;

    const QChar first = t.at(0);
    if (first >= QChar('1') && first <= QChar('4')) return first.digitValue();
    if (first == QChar('0') || normalize(t).startsWith("постійно")) return 0;
    return -1;
}
}

QStringList ScheduleImporter::header() {
    return {"Група", "День", "Пара", "Тиждень", "Предмет", "Викладач", "Аудиторія"};
}

QList<QStringList> ScheduleImporter::toRows(const QList<ScheduleRecord> &records) {
    QList<QStringList> rows;
    rows.append(header());
    for (const auto &r : records) {
        rows.append(QStringList{r.groupName, scheduleDayNames().value(r.dayOfWeek - 1), QString::number(r.pairNumber),
                                weekTypeName(r.weekType), r.subjectName, r.teacherName, r.roomNumber});
    }
    return rows;
}

QList<ImportRow> ScheduleImporter::parse(const QList<QStringList> &rows) {
    QList<ImportRow> result;

    for (int i = 0; i < rows.size(); ++i) {
        const QStringList &cells = rows[i];
        if (i == 0 && normalize(cells.value(0)) == normalize(header().first())) continue;

        ImportRow row;
        row.sourceRow = i + 1;
        row.groupName = cells.value(0).trimmed();
        row.subjectName = cells.value(4).trimmed();
        row.teacherName = cells.value(5).trimmed();
        row.roomName = cells.value(6).trimmed();

        const int day = parseDay(cells.value(1));
        const int pair = cells.value(2).trimmed().toInt();
        const int week = parseWeek(cells.value(3));

        if (row.groupName.isEmpty() || row.subjectName.isEmpty() || row.teacherName.isEmpty() || row.roomName.isEmpty()) {
            row.error = "порожнє поле (група, предмет, викладач або аудиторія)";
        } else if (day == 0) {
            row.error = QString("невірний день тижня «%1»").arg(cells.value(1));
        } else if (pair <= 0) {
            row.error = QString("невірний номер пари «%1»").arg(cells.value(2));
        } else if (week < 0) {
            row.error = QString("невірний тиждень «%1»").arg(cells.value(3));
        }

        row.record.dayOfWeek = day;
        row.record.pairNumber = pair;
        row.record.weekType = week < 0 ? 0 : week;
        result.append(row);
    }
    return result;
}

void ScheduleImporter::resolveReferences(QList<ImportRow> &rows) {
    auto &db = DatabaseManager::instance();

    QHash<QString, int> groups, teachers, subjects, rooms;
    for (const auto &g : db.getGroups()) groups.insert(normalize(g.name), g.id);
    for (const auto &t : db.getTeachers()) teachers.insert(normalize(t.name), t.id);
    for (const auto &s : db.getSubjects()) subjects.insert(normalize(s.name), s.id);
    for (const auto &r : db.getClassrooms()) rooms.insert(normalize(r.roomNumber), r.id);

    for (auto &row : rows) {
        row.missing.clear();
        if (!row.error.isEmpty()) continue;

        auto lookup = [&row](const QHash<QString, int> &map, const QString &name, const QString &label, int &target) {
            target = map.value(normalize(name), 0);
            if (target == 0) row.missing << QString("%1 «%2»").arg(label, name);
        };

        lookup(groups, row.groupName, "група", row.record.groupId);
        lookup(teachers, row.teacherName, "викладач", row.record.teacherId);
        lookup(subjects, row.subjectName, "предмет", row.record.subjectId);
        lookup(rooms, row.roomName, "аудиторія", row.record.classroomId);
    }
}

ImportReport ScheduleImporter::run(QList<ImportRow> &rows, bool replaceGroups, bool createMissing) {
    auto &db = DatabaseManager::instance();
    ImportReport report;

    resolveReferences(rows);
    db.beginTransaction();

    if (createMissing) {
        QSet<QString> created;
        auto once = [&created](const QString &key) {
            if (created.contains(key)) return false;
            created.insert(key);
            return true;
        };

        QString ignored;
        for (const auto &row : rows) {
            if (!row.error.isEmpty()) continue;
            if (row.record.groupId == 0 && once("g:" + normalize(row.groupName)))
                db.saveGroup(0, row.groupName, ignored);
            if (row.record.teacherId == 0 && once("t:" + normalize(row.teacherName)))
                db.saveTeacher(0, row.teacherName, QString(), ignored);
            if (row.record.subjectId == 0 && once("s:" + normalize(row.subjectName)))
                db.saveSubject(0, row.subjectName, "#4CAF50", ignored);
            if (row.record.classroomId == 0 && once("c:" + normalize(row.roomName)))
                db.saveClassroom(0, row.roomName, 30, ignored);
        }
        resolveReferences(rows);
    }

    if (replaceGroups) {
        QSet<int> groupIds;
        for (const auto &row : rows) {
            if (row.error.isEmpty() && row.missing.isEmpty()) groupIds.insert(row.record.groupId);
        }
        for (int id : groupIds) {
            db.deleteScheduleForGroup(id);
        }
    }

    for (const auto &row : rows) {
        ++report.total;

        if (!row.error.isEmpty()) {
            report.errors << QString("Рядок %1: %2").arg(row.sourceRow).arg(row.error);
            continue;
        }
        if (!row.missing.isEmpty()) {
            report.errors << QString("Рядок %1: не знайдено — %2").arg(row.sourceRow).arg(row.missing.join(", "));
            continue;
        }

        QString error;
        if (db.addScheduleRecord(row.record, error)) {
            ++report.imported;
        } else {
            report.errors << QString("Рядок %1: %2").arg(row.sourceRow).arg(error);
        }
    }

    if (!db.commitTransaction()) {
        db.rollbackTransaction();
        report.imported = 0;
        report.errors << "Не вдалося зберегти зміни в базі даних.";
    }
    return report;
}
