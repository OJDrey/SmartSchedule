#ifndef TEACHERSDIALOG_H
#define TEACHERSDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFormLayout>
#include <QLineEdit>
#include "databasemanager.h"

class TeachersDialog : public QDialog {
    Q_OBJECT

public:
    explicit TeachersDialog(QWidget *parent = nullptr);

private slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();

private:
    void loadTeachers();

    QTableWidget *m_tableWidget;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_closeBtn;
};

#endif // TEACHERSDIALOG_H
