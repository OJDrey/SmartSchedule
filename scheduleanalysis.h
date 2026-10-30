#ifndef SCHEDULEANALYSIS_H
#define SCHEDULEANALYSIS_H

#include <QList>
#include <QMap>
#include <QSet>
#include <QTime>
#include <QPainter>
#include <QStyledItemDelegate>
#include "databasemanager.h"
#include "schedulefilter.h"

namespace ScheduleAnalyzer {

struct Slot {
    int day = 0;
    int pair = 0;
};

struct Load {
    double lessons = 0;
    double hours = 0;
    double windows = 0;
};

inline bool occupies(const ScheduleRecord &record, int weekType) {
    return weekType == 0 || record.weekType == 0 || record.weekType == weekType;
}

inline int cycleLength(const QList<ScheduleRecord> &records) {
    int maxType = 0;
    for (const auto &r : records) {
        maxType = qMax(maxType, r.weekType);
    }
    return maxType >= 3 ? 4 : (maxType >= 1 ? 2 : 1);
}

inline QList<Slot> findWindows(const QList<ScheduleRecord> &records, int weekType) {
    QMap<int, QSet<int>> pairsByDay;
    for (const auto &r : records) {
        if (occupies(r, weekType)) pairsByDay[r.dayOfWeek].insert(r.pairNumber);
    }

    QList<Slot> windows;
    for (auto it = pairsByDay.constBegin(); it != pairsByDay.constEnd(); ++it) {
        int first = 1000;
        int last = 0;
        for (int pair : it.value()) {
            first = qMin(first, pair);
            last = qMax(last, pair);
        }
        for (int pair = first + 1; pair < last; ++pair) {
            if (!it.value().contains(pair)) windows.append(Slot{it.key(), pair});
        }
    }
    return windows;
}

inline QMap<int, double> pairHours(const QList<BellItem> &bells) {
    QMap<int, double> hours;
    for (const auto &b : bells) {
        const QTime start = QTime::fromString(b.startTime, "hh:mm");
        const QTime end = QTime::fromString(b.endTime, "hh:mm");
        hours[b.pairNumber] = start.isValid() && end.isValid() ? start.secsTo(end) / 3600.0 : 0.0;
    }
    return hours;
}

struct Conflict {
    QString type;
    int day = 0;
    int pair = 0;
    QString description;
    int groupId = 0;
};

struct RoomLoad {
    double busyPairs = 0;
    double percent = 0;
};

inline bool weeksOverlap(const ScheduleRecord &a, const ScheduleRecord &b) {
    return a.weekType == 0 || b.weekType == 0 || a.weekType == b.weekType;
}

inline QList<Conflict> findConflicts(const QList<ScheduleRecord> &records, const QList<BellItem> &bells) {
    QList<Conflict> result;

    QSet<int> knownPairs;
    for (const auto &b : bells) {
        knownPairs.insert(b.pairNumber);
    }

    QMap<int, QList<ScheduleRecord>> bySlot;
    for (const auto &r : records) {
        bySlot[r.dayOfWeek * 100 + r.pairNumber].append(r);
        if (!knownPairs.contains(r.pairNumber)) {
            result.append(Conflict{"Дзвінки", r.dayOfWeek, r.pairNumber,
                                   QString("Група %1: «%2» призначено на пару, якої немає в розкладі дзвінків.")
                                       .arg(r.groupName, r.subjectName),
                                   r.groupId});
        }
    }

    for (auto it = bySlot.constBegin(); it != bySlot.constEnd(); ++it) {
        const auto &list = it.value();
        const int day = it.key() / 100;
        const int pair = it.key() % 100;

        for (int i = 0; i < list.size(); ++i) {
            for (int j = i + 1; j < list.size(); ++j) {
                const auto &a = list[i];
                const auto &b = list[j];
                if (!weeksOverlap(a, b)) continue;

                const int week = a.weekType != 0 ? a.weekType : b.weekType;
                const QString note = week > 0 ? QString(" [%1 тиждень]").arg(week) : QString();

                if (a.teacherId == b.teacherId) {
                    result.append(Conflict{"Викладач", day, pair,
                                           QString("%1 веде пари одночасно: група %2 та група %3.%4")
                                               .arg(a.teacherName, a.groupName, b.groupName, note),
                                           a.groupId});
                }
                if (a.classroomId == b.classroomId) {
                    result.append(Conflict{"Аудиторія", day, pair,
                                           QString("Аудиторія %1 зайнята одночасно: група %2 та група %3.%4")
                                               .arg(a.roomNumber, a.groupName, b.groupName, note),
                                           a.groupId});
                }
                if (a.groupId == b.groupId) {
                    result.append(Conflict{"Група", day, pair,
                                           QString("Група %1 має дві пари одночасно: «%2» та «%3».%4")
                                               .arg(a.groupName, a.subjectName, b.subjectName, note),
                                           a.groupId});
                }
            }
        }
    }
    return result;
}

inline RoomLoad computeRoomLoad(const QList<ScheduleRecord> &records, int pairsPerDay, int weekType, int cycle) {
    QList<int> weeks;
    if (weekType == 0) {
        for (int w = 1; w <= cycle; ++w) weeks << w;
    } else {
        weeks << weekType;
    }

    double totalSlots = 0;
    for (int w : weeks) {
        QSet<int> usedSlots;
        for (const auto &r : records) {
            if (occupies(r, w)) usedSlots.insert(r.dayOfWeek * 100 + r.pairNumber);
        }
        totalSlots += usedSlots.size();
    }

    RoomLoad load;
    load.busyPairs = totalSlots / weeks.size();
    const double capacity = 5.0 * qMax(1, pairsPerDay);
    load.percent = load.busyPairs / capacity * 100.0;
    return load;
}

inline Load computeLoad(const QList<ScheduleRecord> &records, const QMap<int, double> &hoursByPair, int weekType, int cycle) {
    QList<int> weeks;
    if (weekType == 0) {
        for (int w = 1; w <= cycle; ++w) weeks << w;
    } else {
        weeks << weekType;
    }

    Load load;
    for (int w : weeks) {
        for (const auto &r : records) {
            if (!occupies(r, w)) continue;
            load.lessons += 1;
            load.hours += hoursByPair.value(r.pairNumber);
        }
        load.windows += findWindows(records, w).size();
    }

    const double count = weeks.size();
    load.lessons /= count;
    load.hours /= count;
    load.windows /= count;
    return load;
}

}

class WindowsDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSet<int> windows;

    static int key(int row, int column) { return row * 100 + column; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyledItemDelegate::paint(painter, option, index);

        if (!windows.contains(key(index.row(), index.column()))) return;

        painter->save();
        painter->fillRect(option.rect, QBrush(QColor(244, 67, 54, 90), Qt::BDiagPattern));
        painter->setPen(QColor(244, 67, 54));
        painter->drawText(option.rect, Qt::AlignCenter, "вікно");
        painter->restore();
    }
};

#endif // SCHEDULEANALYSIS_H
