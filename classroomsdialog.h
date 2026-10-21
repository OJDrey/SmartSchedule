#ifndef CLASSROOMSDIALOG_H
#define CLASSROOMSDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include "databasemanager.h"

class ClassroomsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ClassroomsDialog(QWidget *parent = nullptr);

private slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();

private:
    void loadClassrooms();

    QTableWidget *m_tableWidget;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_closeBtn;
};

#endif // CLASSROOMSDIALOG_H
