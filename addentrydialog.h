#ifndef ADDENTRYDIALOG_H
#define ADDENTRYDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include "databasemanager.h"

class AddEntryDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddEntryDialog(int currentGroupId, QWidget *parent = nullptr);
    ScheduleRecord getRecord() const;

private slots:
    void onSaveClicked();

private:
    int m_groupId;
    QComboBox *m_dayCombo;
    QComboBox *m_pairCombo;
    QComboBox *m_weekTypeCombo;
    QComboBox *m_subjectCombo;
    QComboBox *m_teacherCombo;
    QComboBox *m_roomCombo;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;

    ScheduleRecord m_createdRecord;
};

#endif // ADDENTRYDIALOG_H
