#ifndef SCHEDULEFILTER_H
#define SCHEDULEFILTER_H

#include <QString>
#include <QStringList>
#include "databasemanager.h"

enum class ViewMode { Group = 0, Teacher = 1, Classroom = 2 };

inline Entity entityOf(ViewMode mode) {
    switch (mode) {
    case ViewMode::Teacher: return Entity::Teacher;
    case ViewMode::Classroom: return Entity::Classroom;
    default: return Entity::Group;
    }
}

inline QString viewModeTitle(ViewMode mode) {
    switch (mode) {
    case ViewMode::Teacher: return "Розклад викладача";
    case ViewMode::Classroom: return "Розклад аудиторії";
    default: return "Розклад групи";
    }
}

inline const QStringList &scheduleDayNames() {
    static const QStringList names = {"Понеділок", "Вівторок", "Середа", "Четвер", "П'ятниця"};
    return names;
}

struct ScheduleFilter {
    QString text;
    int teacherId = 0;
    int subjectId = 0;
    int classroomId = 0;

    bool isActive() const {
        return !text.trimmed().isEmpty() || teacherId > 0 || subjectId > 0 || classroomId > 0;
    }

    bool matches(const ScheduleRecord &r) const {
        if (teacherId > 0 && r.teacherId != teacherId) return false;
        if (subjectId > 0 && r.subjectId != subjectId) return false;
        if (classroomId > 0 && r.classroomId != classroomId) return false;

        const QString needle = text.trimmed();
        if (needle.isEmpty()) return true;
        return r.subjectName.contains(needle, Qt::CaseInsensitive)
               || r.teacherName.contains(needle, Qt::CaseInsensitive)
               || r.roomNumber.contains(needle, Qt::CaseInsensitive)
               || r.groupName.contains(needle, Qt::CaseInsensitive);
    }
};

inline QString recordTitle(const ScheduleRecord &r) {
    const QString prefix = r.weekType > 0 ? QString("[%1 тижд] ").arg(r.weekType) : QString();
    return prefix + r.subjectName;
}

inline QString recordDetails(const ScheduleRecord &r, ViewMode mode) {
    switch (mode) {
    case ViewMode::Teacher: return QString("Група: %1 | Ауд: %2").arg(r.groupName, r.roomNumber);
    case ViewMode::Classroom: return QString("Група: %1 | Викл: %2").arg(r.groupName, r.teacherName);
    default: return QString("Викл: %1 | Ауд: %2").arg(r.teacherName, r.roomNumber);
    }
}

inline QString recordText(const ScheduleRecord &r, ViewMode mode) {
    return recordTitle(r) + kNewLine + recordDetails(r, mode);
}

#endif // SCHEDULEFILTER_H
