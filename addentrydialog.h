#ifndef ADDENTRYDIALOG_H
#define ADDENTRYDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QInputDialog>
#include "databasemanager.h"

class AddEntryDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddEntryDialog(int currentGroupId, QWidget *parent = nullptr, int editRecordId = 0,
                            int presetDay = 0, int presetPair = 0);
    ScheduleRecord getRecord() const;

private slots:
    void onSaveClicked();
    void onFindRoomClicked();

private:
    int m_groupId;
    int m_recordId;
    QComboBox *m_dayCombo;
    QComboBox *m_pairCombo;
    QComboBox *m_weekTypeCombo;
    QComboBox *m_subjectCombo;
    QComboBox *m_teacherCombo;
    QComboBox *m_roomCombo;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_findRoomBtn;

    ScheduleRecord m_createdRecord;
};

#endif // ADDENTRYDIALOG_H
