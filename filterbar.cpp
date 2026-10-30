#include "filterbar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>

FilterBar::FilterBar(QWidget *parent) : QWidget(parent) {
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔎 Пошук за предметом, викладачем, групою чи аудиторією...");
    m_searchEdit->setClearButtonEnabled(true);

    m_teacherCombo = new QComboBox(this);
    m_subjectCombo = new QComboBox(this);
    m_roomCombo = new QComboBox(this);
    m_clearBtn = new QPushButton("Скинути фільтр", this);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_searchEdit, 2);
    layout->addWidget(m_teacherCombo, 1);
    layout->addWidget(m_subjectCombo, 1);
    layout->addWidget(m_roomCombo, 1);
    layout->addWidget(m_clearBtn);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &FilterBar::filterChanged);
    connect(m_teacherCombo, &QComboBox::currentIndexChanged, this, &FilterBar::filterChanged);
    connect(m_subjectCombo, &QComboBox::currentIndexChanged, this, &FilterBar::filterChanged);
    connect(m_roomCombo, &QComboBox::currentIndexChanged, this, &FilterBar::filterChanged);
    connect(m_clearBtn, &QPushButton::clicked, this, [this] {
        {
            const QSignalBlocker b1(m_searchEdit);
            const QSignalBlocker b2(m_teacherCombo);
            const QSignalBlocker b3(m_subjectCombo);
            const QSignalBlocker b4(m_roomCombo);
            m_searchEdit->clear();
            m_teacherCombo->setCurrentIndex(0);
            m_subjectCombo->setCurrentIndex(0);
            m_roomCombo->setCurrentIndex(0);
        }
        emit filterChanged();
    });

    reloadLists();
}

ScheduleFilter FilterBar::filter() const {
    ScheduleFilter f;
    f.text = m_searchEdit->text();
    f.teacherId = m_teacherCombo->currentData().toInt();
    f.subjectId = m_subjectCombo->currentData().toInt();
    f.classroomId = m_roomCombo->currentData().toInt();
    return f;
}

void FilterBar::fillCombo(QComboBox *combo, const QString &allText, const QList<QPair<int, QString>> &items) {
    const QSignalBlocker blocker(combo);
    const int previousId = combo->currentData().toInt();

    combo->clear();
    combo->addItem(allText, 0);
    for (const auto &item : items) {
        combo->addItem(item.second, item.first);
    }

    const int index = combo->findData(previousId);
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

void FilterBar::reloadLists() {
    auto &db = DatabaseManager::instance();

    QList<QPair<int, QString>> teachers, subjects, rooms;
    for (const auto &t : db.getTeachers()) teachers.append(qMakePair(t.id, t.name));
    for (const auto &s : db.getSubjects()) subjects.append(qMakePair(s.id, s.name));
    for (const auto &r : db.getClassrooms()) rooms.append(qMakePair(r.id, QString("Ауд. %1").arg(r.roomNumber)));

    fillCombo(m_teacherCombo, "Усі викладачі", teachers);
    fillCombo(m_subjectCombo, "Усі предмети", subjects);
    fillCombo(m_roomCombo, "Усі аудиторії", rooms);
}
