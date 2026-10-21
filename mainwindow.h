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
#include <QTimer>
#include <QMap>
#include <QPair>
#include <functional>
#include "databasemanager.h"
#include "addentrydialog.h"
#include "bellsdialog.h"
#include "referencedialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onAddEntryClicked();
    void onDeleteEntryClicked();
    void onOpenBellsDialog();
    void onOpenAboutDialog();
    void onExportPdf();
    void refreshScheduleTable();
    void updateCurrentWeekInfo();

private:
    void setupUi();
    void loadGroups();
    void addReferenceAction(QMenu *menu, const QString &text, std::function<ReferenceConfig()> makeConfig);

    QLabel *m_statusWeekLabel;
    QComboBox *m_groupCombo;
    QComboBox *m_weekFilterCombo;
    QPushButton *m_addBtn;
    QPushButton *m_deleteBtn;
    QTableWidget *m_tableWidget;
};

#endif // MAINWINDOW_H

