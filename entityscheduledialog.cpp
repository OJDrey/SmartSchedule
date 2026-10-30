#include "entityscheduledialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

EntityScheduleDialog::EntityScheduleDialog(ViewMode mode, int entityId, const QString &name, QWidget *parent)
    : QDialog(parent), m_mode(mode), m_entityId(entityId)
{
    setWindowTitle(viewModeTitle(mode) + ": " + name);
    resize(980, 620);

    m_weekCombo = new QComboBox(this);
    m_weekCombo->addItem("📅 Усі тижні", 0);
    for (int w = 1; w <= 4; ++w) {
        m_weekCombo->addItem(weekTypeName(w), w);
    }

    m_windowsLabel = new QLabel(this);
    m_windowsLabel->setStyleSheet("font-weight: bold;");

    m_grid = new ScheduleGrid(this);

    auto *closeBtn = new QPushButton("Закрити", this);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Фільтр тижня:", this));
    topLayout->addWidget(m_weekCombo);
    topLayout->addWidget(m_windowsLabel, 1);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_grid);
    mainLayout->addWidget(closeBtn);

    connect(m_weekCombo, &QComboBox::currentIndexChanged, this, &EntityScheduleDialog::reload);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    reload();
}

void EntityScheduleDialog::reload() {
    auto &db = DatabaseManager::instance();
    const int week = m_weekCombo->currentData().toInt();
    const auto records = db.getScheduleForEntity(entityOf(m_mode), m_entityId, week);

    const bool windows = m_mode != ViewMode::Classroom;
    m_grid->setBells(db.getBellSchedule());
    m_grid->showRecords(records, m_mode, ScheduleFilter(), windows, week);
    m_windowsLabel->setText(windows ? QString("Вікон у розкладі: %1").arg(m_grid->windowsCount()) : QString());
}
