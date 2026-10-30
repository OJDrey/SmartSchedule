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

struct EntryPreset {
    int day = 0;
    int pair = 0;
    int teacherId = 0;
    int classroomId = 0;
};

class AddEntryDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddEntryDialog(int groupId, QWidget *parent = nullptr, int editRecordId = 0,
                            const EntryPreset &preset = EntryPreset());
    ScheduleRecord getRecord() const;

private slots:
    void onSaveClicked();
    void onFindRoomClicked();

private:
    int m_groupId;
    int m_recordId;
    QComboBox *m_groupCombo = nullptr;
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
