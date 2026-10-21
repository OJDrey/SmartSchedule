#include "addentrydialog.h"

AddEntryDialog::AddEntryDialog(int currentGroupId, QWidget *parent)
    : QDialog(parent), m_groupId(currentGroupId)
{
    setWindowTitle("Додати пару у розклад");
    resize(400, 300);

    auto &db = DatabaseManager::instance();

    m_dayCombo = new QComboBox(this);
    m_dayCombo->addItems({"Понеділок", "Вівторок", "Середа", "Четвер", "П'ятниця"});

    m_pairCombo = new QComboBox(this);
    for (const auto &b : db.getBellSchedule()) {
        m_pairCombo->addItem(QString("%1 пара (%2 - %3)").arg(b.pairNumber).arg(b.startTime, b.endTime), b.pairNumber);
    }

    m_weekTypeCombo = new QComboBox(this);
    for (int w = 0; w <= 4; ++w) {
        m_weekTypeCombo->addItem(weekTypeName(w), w);
    }

    m_subjectCombo = new QComboBox(this);
    for (const auto &s : db.getSubjects()) {
        m_subjectCombo->addItem(s.name, s.id);
    }

    m_teacherCombo = new QComboBox(this);
    for (const auto &t : db.getTeachers()) {
        m_teacherCombo->addItem(t.name, t.id);
    }

    m_roomCombo = new QComboBox(this);
    for (const auto &r : db.getClassrooms()) {
        m_roomCombo->addItem(QString("Ауд. %1 (місткість: %2)").arg(r.roomNumber).arg(r.capacity), r.id);
    }

    auto *formLayout = new QFormLayout();
    formLayout->addRow("День тижня:", m_dayCombo);
    formLayout->addRow("Номер пари:", m_pairCombo);
    formLayout->addRow("Цикл тижня:", m_weekTypeCombo);
    formLayout->addRow("Предмет:", m_subjectCombo);
    formLayout->addRow("Викладач:", m_teacherCombo);
    formLayout->addRow("Аудиторія:", m_roomCombo);

    m_saveBtn = new QPushButton("Зберегти", this);
    m_saveBtn->setDefault(true);
    m_cancelBtn = new QPushButton("Скасувати", this);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_cancelBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);

    const bool hasData = m_pairCombo->count() && m_subjectCombo->count()
                         && m_teacherCombo->count() && m_roomCombo->count();
    m_saveBtn->setEnabled(hasData);

    connect(m_saveBtn, &QPushButton::clicked, this, &AddEntryDialog::onSaveClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AddEntryDialog::onSaveClicked() {
    ScheduleRecord record;
    record.groupId = m_groupId;
    record.dayOfWeek = m_dayCombo->currentIndex() + 1;
    record.pairNumber = m_pairCombo->currentData().toInt();
    record.weekType = m_weekTypeCombo->currentData().toInt();
    record.subjectId = m_subjectCombo->currentData().toInt();
    record.teacherId = m_teacherCombo->currentData().toInt();
    record.classroomId = m_roomCombo->currentData().toInt();

    QString errorMsg;
    if (!DatabaseManager::instance().addScheduleRecord(record, errorMsg)) {
        QMessageBox::critical(this, "Помилка створення", errorMsg);
        return;
    }

    m_createdRecord = record;
    accept();
}

ScheduleRecord AddEntryDialog::getRecord() const {
    return m_createdRecord;
}
