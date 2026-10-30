#include "schedulegrid.h"
#include <QHeaderView>
#include <QDate>
#include <QTime>
#include <QMap>
#include <QPair>

namespace {
constexpr int IdsRole = Qt::UserRole;
constexpr int LabelsRole = Qt::UserRole + 1;
constexpr int DaysCount = 5;
}

ScheduleGrid::ScheduleGrid(QWidget *parent) : QTableWidget(0, DaysCount, parent) {
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::SingleSelection);

    m_delegate = new HighlightDelegate(this);
    setItemDelegate(m_delegate);
    setHorizontalHeaderLabels(scheduleDayNames());
}

void ScheduleGrid::setBells(const QList<BellItem> &bells) {
    m_bells = bells;
    setRowCount(m_bells.size());

    m_pairHeaders.clear();
    for (const auto &b : m_bells) {
        m_pairHeaders << QString("%1 пара").arg(b.pairNumber) + kNewLine + QString("%1 - %2").arg(b.startTime, b.endTime);
    }
    refreshHighlight();
}

void ScheduleGrid::refreshHighlight() {
    const int dayOfWeek = QDate::currentDate().dayOfWeek();
    const int todayColumn = dayOfWeek <= DaysCount ? dayOfWeek - 1 : -1;

    int currentRow = -1;
    if (todayColumn >= 0) {
        const QTime now = QTime::currentTime();
        for (int i = 0; i < m_bells.size(); ++i) {
            const QTime start = QTime::fromString(m_bells[i].startTime, "hh:mm");
            const QTime end = QTime::fromString(m_bells[i].endTime, "hh:mm");
            if (now >= start && now <= end) {
                currentRow = i;
                break;
            }
        }
    }

    QStringList days = scheduleDayNames();
    if (todayColumn >= 0) days[todayColumn] = "▶ " + days[todayColumn] + " (сьогодні)";
    setHorizontalHeaderLabels(days);

    QStringList pairs = m_pairHeaders;
    if (currentRow >= 0 && currentRow < pairs.size()) pairs[currentRow] = "▶ " + pairs[currentRow];
    setVerticalHeaderLabels(pairs);

    m_delegate->todayColumn = todayColumn;
    m_delegate->currentRow = currentRow;
    viewport()->update();
}

void ScheduleGrid::showRecords(const QList<ScheduleRecord> &records, ViewMode mode, const ScheduleFilter &filter,
                               bool showWindows, int weekType) {
    clearContents();
    m_delegate->windows.clear();
    m_windowsCount = 0;

    QMap<QPair<int, int>, QList<ScheduleRecord>> cells;
    for (const auto &rec : records) {
        cells[qMakePair(rec.pairNumber - 1, rec.dayOfWeek - 1)].append(rec);
    }

    const bool filterActive = filter.isActive();
    const QString separator = QString(kNewLine) + "-----------------" + kNewLine;

    for (auto it = cells.constBegin(); it != cells.constEnd(); ++it) {
        const int row = it.key().first;
        const int col = it.key().second;
        const auto &list = it.value();
        if (row < 0 || row >= m_bells.size() || col < 0 || col >= DaysCount) continue;

        QStringList lines, labels;
        QVariantList ids;
        bool anyMatch = !filterActive;
        for (const auto &rec : list) {
            lines << recordText(rec, mode);
            labels << recordTitle(rec) + " — " + recordDetails(rec, mode);
            ids << rec.id;
            if (filterActive && filter.matches(rec)) anyMatch = true;
        }

        auto *item = new QTableWidgetItem(lines.join(separator));
        item->setTextAlignment(Qt::AlignCenter);

        QColor background;
        QColor foreground;
        if (!anyMatch) {
            background = QColor("#CFD8DC");
            foreground = QColor("#78909C");
        } else {
            background = list.size() > 1 ? QColor("#E3F2FD") : QColor(list.first().colorHex);
            if (!background.isValid()) background = QColor("#4CAF50");
            foreground = background.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black);
        }

        item->setBackground(background);
        item->setForeground(foreground);
        item->setData(IdsRole, ids);
        item->setData(LabelsRole, labels);
        setItem(row, col, item);
    }

    if (showWindows) {
        const auto windows = ScheduleAnalyzer::findWindows(records, weekType);
        m_windowsCount = windows.size();
        for (const auto &w : windows) {
            m_delegate->windows.insert(WindowsDelegate::key(w.pair - 1, w.day - 1));
        }
    }
    viewport()->update();
}

int ScheduleGrid::pairNumberAt(int row) const {
    return row >= 0 && row < m_bells.size() ? m_bells[row].pairNumber : 0;
}

QVariantList ScheduleGrid::recordIdsAt(int row, int column) const {
    const auto *cell = item(row, column);
    return cell ? cell->data(IdsRole).toList() : QVariantList();
}

QStringList ScheduleGrid::recordLabelsAt(int row, int column) const {
    const auto *cell = item(row, column);
    return cell ? cell->data(LabelsRole).toStringList() : QStringList();
}
