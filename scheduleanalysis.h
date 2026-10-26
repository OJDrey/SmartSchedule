#ifndef SCHEDULEANALYSIS_H
#define SCHEDULEANALYSIS_H

#include <QList>
#include <QMap>
#include <QSet>
#include <QTime>
#include <QPainter>
#include <QStyledItemDelegate>
#include "databasemanager.h"

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