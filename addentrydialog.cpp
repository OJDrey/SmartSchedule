#include "addentrydialog.h"

AddEntryDialog::AddEntryDialog(int currentGroupId, QWidget *parent, int editRecordId, int presetDay, int presetPair)
    : QDialog(parent), m_groupId(currentGroupId), m_recordId(editRecordId)
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

    if (m_recordId > 0) {
        setWindowTitle("Редагування пари");

        ScheduleRecord existing;
        if (db.getScheduleRecord(m_recordId, existing)) {
            auto select = [](QComboBox *combo, int value) {
                const int index = combo->findData(value);
                if (index >= 0) combo->setCurrentIndex(index);
            };
            m_dayCombo->setCurrentIndex(qBound(0, existing.dayOfWeek - 1, m_dayCombo->count() - 1));
            select(m_pairCombo, existing.pairNumber);
            select(m_weekTypeCombo, existing.weekType);
            select(m_subjectCombo, existing.subjectId);
            select(m_teacherCombo, existing.teacherId);
            select(m_roomCombo, existing.classroomId);
        }
    }

    if (m_recordId <= 0 && presetDay > 0) {
        m_dayCombo->setCurrentIndex(qBound(0, presetDay - 1, m_dayCombo->count() - 1));
        const int pairIndex = m_pairCombo->findData(presetPair);
        if (pairIndex >= 0) m_pairCombo->setCurrentIndex(pairIndex);
    }

    auto *formLayout = new QFormLayout();
    formLayout->addRow("День тижня:", m_dayCombo);
    formLayout->addRow("Номер пари:", m_pairCombo);
    formLayout->addRow("Цикл тижня:", m_weekTypeCombo);
    formLayout->addRow("Предмет:", m_subjectCombo);
    formLayout->addRow("Викладач:", m_teacherCombo);
    m_findRoomBtn = new QPushButton("🔍 Вільні", this);
    auto *roomRow = new QHBoxLayout();
    roomRow->addWidget(m_roomCombo, 1);
    roomRow->addWidget(m_findRoomBtn);
    formLayout->addRow("Аудиторія:", roomRow);

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
    connect(m_findRoomBtn, &QPushButton::clicked, this, &AddEntryDialog::onFindRoomClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AddEntryDialog::onSaveClicked() {
    ScheduleRecord record;
    record.id = m_recordId;
    record.groupId = m_groupId;
    record.dayOfWeek = m_dayCombo->currentIndex() + 1;
    record.pairNumber = m_pairCombo->currentData().toInt();
    record.weekType = m_weekTypeCombo->currentData().toInt();
    record.subjectId = m_subjectCombo->currentData().toInt();
    record.teacherId = m_teacherCombo->currentData().toInt();
    record.classroomId = m_roomCombo->currentData().toInt();

    QString errorMsg;
    auto &db = DatabaseManager::instance();
    const bool saved = m_recordId > 0 ? db.updateScheduleRecord(record, errorMsg) : db.addScheduleRecord(record, errorMsg);
    if (!saved) {
        QMessageBox::critical(this, m_recordId > 0 ? "Помилка редагування" : "Помилка створення", errorMsg);
        return;
    }

    m_createdRecord = record;
    accept();
}

void AddEntryDialog::onFindRoomClicked() {
    const auto rooms = DatabaseManager::instance().getFreeClassrooms(
        m_dayCombo->currentIndex() + 1, m_pairCombo->currentData().toInt(),
        m_weekTypeCombo->currentData().toInt(), m_recordId);

    if (rooms.isEmpty()) {
        QMessageBox::information(this, "Вільні аудиторії", "На вибраний час усі аудиторії зайняті.");
        return;
    }

    QStringList labels;
    for (const auto &r : rooms) {
        labels << QString("Ауд. %1 (місткість: %2)").arg(r.roomNumber).arg(r.capacity);
    }

    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, "Вільні аудиторії",
                                                 "Оберіть аудиторію (від найменшої місткості):", labels, 0, false, &ok);
    if (!ok) return;

    const int index = m_roomCombo->findData(rooms.at(qMax(0, labels.indexOf(chosen))).id);
    if (index >= 0) m_roomCombo->setCurrentIndex(index);
}

ScheduleRecord AddEntryDialog::getRecord() const {
    return m_createdRecord;
}
