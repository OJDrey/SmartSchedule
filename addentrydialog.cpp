#include "addentrydialog.h"

AddEntryDialog::AddEntryDialog(int currentGroupId, QWidget *parent)
    : QDialog(parent), m_groupId(currentGroupId)
{
    setWindowTitle("Додати пару у розклад");
    resize(400, 300);

    QFormLayout *formLayout = new QFormLayout();

    m_dayCombo = new QComboBox(this);
    m_dayCombo->addItems({"Понеділок", "Вівторок", "Середа", "Четвер", "П'ятниця"});

    m_pairCombo = new QComboBox(this);
    auto bells = DatabaseManager::instance().getBellSchedule();
    for (const auto &b : bells) {
        m_pairCombo->addItem(QString("%1 пара (%2 - %3)").arg(b.pairNumber).arg(b.startTime).arg(b.endTime), b.pairNumber);
    }

    m_weekTypeCombo = new QComboBox(this);
    m_weekTypeCombo->addItem("Постійно (Кожного тижня)", 0);
    m_weekTypeCombo->addItem("1-й тиждень (Чисельник)", 1);
    m_weekTypeCombo->addItem("2-й тиждень (Знаменник)", 2);
    m_weekTypeCombo->addItem("3-й тиждень", 3);
    m_weekTypeCombo->addItem("4-й тиждень", 4);

    m_subjectCombo = new QComboBox(this);
    for (const auto &sub : DatabaseManager::instance().getSubjects()) {
        m_subjectCombo->addItem(sub.name, sub.id);
    }

    m_teacherCombo = new QComboBox(this);
    for (const auto &t : DatabaseManager::instance().getTeachers()) {
        m_teacherCombo->addItem(t.name, t.id);
    }

    m_roomCombo = new QComboBox(this);
    for (const auto &r : DatabaseManager::instance().getClassrooms()) {
        m_roomCombo->addItem(QString("Ауд. %1 (місткість: %2)").arg(r.roomNumber).arg(r.capacity), r.id);
    }

    formLayout->addRow("День тижня:", m_dayCombo);
    formLayout->addRow("Номер пари:", m_pairCombo);
    formLayout->addRow("Цикл тижня:", m_weekTypeCombo);
    formLayout->addRow("Предмет:", m_subjectCombo);
    formLayout->addRow("Викладач:", m_teacherCombo);
    formLayout->addRow("Аудиторія:", m_roomCombo);

    m_saveBtn = new QPushButton("Зберегти", this);
    m_cancelBtn = new QPushButton("Скасувати", this);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_cancelBtn);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);

    connect(m_saveBtn, &QPushButton::clicked, this, &AddEntryDialog::onSaveClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AddEntryDialog::onSaveClicked() {
    m_createdRecord.groupId = m_groupId;
    m_createdRecord.dayOfWeek = m_dayCombo->currentIndex() + 1;
    m_createdRecord.pairNumber = m_pairCombo->currentData().toInt();
    m_createdRecord.weekType = m_weekTypeCombo->currentData().toInt();
    m_createdRecord.subjectId = m_subjectCombo->currentData().toInt();
    m_createdRecord.teacherId = m_teacherCombo->currentData().toInt();
    m_createdRecord.classroomId = m_roomCombo->currentData().toInt();

    QString errorMsg;
    if (!DatabaseManager::instance().addScheduleRecord(m_createdRecord, errorMsg)) {
        QMessageBox::critical(this, "Помилка створення", errorMsg);
        return;
    }

    QMessageBox::information(this, "Успіх", "Пару успішно додано до розкладу!");
    accept();
}

ScheduleRecord AddEntryDialog::getRecord() const {
    return m_createdRecord;
}