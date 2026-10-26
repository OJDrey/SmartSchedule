#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDate>
#include <QDateTime>
#include <QCheckBox>
#include <QTime>
#include <QTimer>
#include <QMap>
#include <QPair>
#include <QPainter>
#include <QStyledItemDelegate>
#include <functional>
#include "databasemanager.h"
#include "addentrydialog.h"
#include "bellsdialog.h"
#include "referencedialog.h"
#include "scheduleanalysis.h"
#include "workloaddialog.h"

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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onAddEntryClicked();
    void onDeleteEntryClicked();
    void onCellDoubleClicked(int row, int column);
    void onOpenBellsDialog();
    void onOpenAboutDialog();
    void onExportPdf();
    void onExportTable();
    void onPrint();
    void onShowWorkload();
    void onBackupDatabase();
    void onRestoreDatabase();
    void refreshScheduleTable();
    void updateCurrentWeekInfo();
    void updateHighlight();

private:
    void setupUi();
    void setupMenus();
    void loadGroups();
    bool ensureGroupSelected();
    int pickRecordId(const QTableWidgetItem *item, const QString &title, const QString &prompt);
    QString buildScheduleHtml() const;
    QList<QStringList> scheduleRows() const;
    bool saveScheduleXlsx(const QString &path) const;
    void addReferenceAction(QMenu *menu, const QString &text, std::function<ReferenceConfig()> makeConfig);

    QLabel *m_statusWeekLabel;
    QComboBox *m_groupCombo;
    QComboBox *m_weekFilterCombo;
    QPushButton *m_addBtn;
    QPushButton *m_deleteBtn;
    QTableWidget *m_tableWidget;
    QCheckBox *m_windowsCheck;
    QLabel *m_windowsLabel;
    HighlightDelegate *m_delegate;
    QList<BellItem> m_bells;
    QStringList m_pairHeaders;
};

#endif // MAINWINDOW_H