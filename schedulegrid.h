#ifndef SCHEDULEGRID_H
#define SCHEDULEGRID_H

#include <QTableWidget>
#include <QPainter>
#include <QVariantList>
#include "databasemanager.h"
#include "schedulefilter.h"
#include "scheduleanalysis.h"

class HighlightDelegate : public WindowsDelegate {
public:
    using WindowsDelegate::WindowsDelegate;

    int todayColumn = -1;
    int currentRow = -1;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        WindowsDelegate::paint(painter, option, index);

        painter->save();
        if (index.column() == todayColumn)
            painter->fillRect(option.rect, QColor(255, 193, 7, 45));
        if (index.row() == currentRow)
            painter->fillRect(option.rect, QColor(33, 150, 243, 45));
        if (index.column() == todayColumn && index.row() == currentRow) {
            painter->setPen(QPen(QColor(244, 67, 54), 3));
            painter->drawRect(option.rect.adjusted(1, 1, -2, -2));
        }
        painter->restore();
    }
};

class ScheduleGrid : public QTableWidget {
    Q_OBJECT

public:
    explicit ScheduleGrid(QWidget *parent = nullptr);

    void setBells(const QList<BellItem> &bells);
    void showRecords(const QList<ScheduleRecord> &records, ViewMode mode, const ScheduleFilter &filter,
                     bool showWindows, int weekType);
    void refreshHighlight();

    int windowsCount() const { return m_windowsCount; }
    int pairNumberAt(int row) const;
    QString pairHeaderAt(int row) const { return m_pairHeaders.value(row); }
    QVariantList recordIdsAt(int row, int column) const;
    QStringList recordLabelsAt(int row, int column) const;

private:
    HighlightDelegate *m_delegate;
    QList<BellItem> m_bells;
    QStringList m_pairHeaders;
    int m_windowsCount = 0;
};

#endif // SCHEDULEGRID_H
