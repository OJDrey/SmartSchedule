#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDate>
#include <QDateTime>
#include <QTimer>
#include <functional>
#include "databasemanager.h"
#include "addentrydialog.h"
#include "bellsdialog.h"
#include "referencedialog.h"
#include "schedulegrid.h"
#include "filterbar.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onModeChanged();
    void onAddEntryClicked();
    void onDeleteEntryClicked();
    void onCellDoubleClicked(int row, int column);
    void onOpenBellsDialog();
    void onOpenAboutDialog();
    void onExportPdf();
    void onExportAllGroupsPdf();
    void onExportTable();
    void onExportAllData();
    void onImportSchedule();
    void onPrint();
    void onPrintAllGroups();
    void onShowWorkload();
    void onShowRoomLoad();
    void onShowConflicts();
    void onBackupDatabase();
    void onRestoreDatabase();
    void refreshScheduleTable();
    void updateCurrentWeekInfo();

private:
    void setupUi();
    void setupMenus();
    void loadEntities(int preferredId = 0);
    bool ensureEntitySelected();
    ViewMode currentMode() const;
    int currentEntityId() const;
    int pickRecordId(int row, int column, const QString &title, const QString &prompt);
    void openEntryDialog(int recordId, int day, int pair);
    void showGroup(int groupId);
    QString weekFilterTitle() const;
    QString currentHtml() const;
    QString allGroupsHtml() const;
    void showPrintPreview(const QString &html);
    bool saveScheduleXlsx(const QString &path) const;
    void addReferenceAction(QMenu *menu, const QString &text, std::function<ReferenceConfig()> makeConfig);

    QLabel *m_statusWeekLabel;
    QComboBox *m_modeCombo;
    QLabel *m_entityLabel;
    QComboBox *m_entityCombo;
    QComboBox *m_weekFilterCombo;
    QCheckBox *m_windowsCheck;
    QLabel *m_windowsLabel;
    QPushButton *m_addBtn;
    QPushButton *m_deleteBtn;
    FilterBar *m_filterBar;
    ScheduleGrid *m_grid;

    QList<BellItem> m_bells;
    QList<ScheduleRecord> m_records;
    int m_lastIds[3] = {0, 0, 0};
    int m_previousMode = 0;
};

#endif // MAINWINDOW_H
