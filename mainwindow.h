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
#include <QHeaderView>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDate>
#include "databasemanager.h"
#include "addentrydialog.h"
#include "bellsdialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onGroupChanged(int index);
    void onWeekFilterChanged(int index);
    void onAddEntryClicked();
    void onDeleteEntryClicked();
    void onOpenBellsDialog();
    void refreshScheduleTable();

private:
    void setupUi();
    void loadGroups();
    void updateCurrentWeekInfo();

    QLabel *m_statusWeekLabel;
    QComboBox *m_groupCombo;
    QComboBox *m_weekFilterCombo;
    QPushButton *m_addBtn;
    QPushButton *m_deleteBtn;
    QTableWidget *m_tableWidget;
};

#endif // MAINWINDOW_H