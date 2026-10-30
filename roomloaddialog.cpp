#include "roomloaddialog.h"
#include "entityscheduledialog.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <algorithm>

RoomLoadDialog::RoomLoadDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Завантаженість аудиторій");
    resize(820, 640);

    m_weekCombo = new QComboBox(this);
    m_weekCombo->addItem("Середнє за цикл тижнів", 0);
    for (int w = 1; w <= 4; ++w) {
        m_weekCombo->addItem(weekTypeName(w), w);
    }

    m_infoLabel = new QLabel(this);

    m_tableWidget = new QTableWidget(0, 4, this);
    m_tableWidget->setHorizontalHeaderLabels({"Аудиторія", "Місткість", "Зайнято пар / тиждень", "Завантаженість, %"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_chart = new BarChartWidget(this);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_chart);

    auto *scheduleBtn = new QPushButton("📅 Розклад аудиторії...", this);
    auto *closeBtn = new QPushButton("Закрити", this);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Тиждень:", this));
    topLayout->addWidget(m_weekCombo);
    topLayout->addStretch(1);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(scheduleBtn);
    bottomLayout->addStretch(1);
    bottomLayout->addWidget(closeBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addWidget(m_tableWidget, 2);
    mainLayout->addWidget(scroll, 3);
    mainLayout->addLayout(bottomLayout);

    connect(m_weekCombo, &QComboBox::currentIndexChanged, this, &RoomLoadDialog::reload);
    connect(scheduleBtn, &QPushButton::clicked, this, &RoomLoadDialog::onShowScheduleClicked);
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &RoomLoadDialog::onShowScheduleClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    reload();
}

void RoomLoadDialog::reload() {
    auto &db = DatabaseManager::instance();
    const auto rooms = db.getClassrooms();
    const int pairsPerDay = db.getBellSchedule().size();

    QMap<int, QList<ScheduleRecord>> records;
    int cycle = 1;
    for (const auto &r : rooms) {
        records[r.id] = db.getScheduleForClassroom(r.id, 0);
        cycle = qMax(cycle, ScheduleAnalyzer::cycleLength(records[r.id]));
    }

    const int week = m_weekCombo->currentData().toInt();
    m_rows.clear();
    for (const auto &r : rooms) {
        Row row;
        row.roomId = r.id;
        row.room = r.roomNumber;
        row.capacity = r.capacity;
        row.load = ScheduleAnalyzer::computeRoomLoad(records[r.id], pairsPerDay, week, cycle);
        m_rows.append(row);
    }

    std::sort(m_rows.begin(), m_rows.end(), [](const Row &a, const Row &b) {
        return a.load.percent > b.load.percent;
    });

    auto centered = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(m_rows.size());

    QStringList labels;
    QList<double> values;
    for (int i = 0; i < m_rows.size(); ++i) {
        const Row &r = m_rows[i];
        auto *nameItem = new QTableWidgetItem(QString("Ауд. %1").arg(r.room));
        nameItem->setData(Qt::UserRole, r.roomId);
        m_tableWidget->setItem(i, 0, nameItem);
        m_tableWidget->setItem(i, 1, centered(QString::number(r.capacity)));
        m_tableWidget->setItem(i, 2, centered(QString::number(r.load.busyPairs, 'f', 1)));
        m_tableWidget->setItem(i, 3, centered(QString::number(r.load.percent, 'f', 1) + "%"));

        labels << QString("Ауд. %1 (місць: %2)").arg(r.room).arg(r.capacity);
        values << r.load.percent;
    }

    m_infoLabel->setText(QString("Усього слотів на тиждень в одній аудиторії: %1 (5 днів × %2 пар). "
                                 "Завантаженість = зайняті слоти / усі слоти.")
                             .arg(5 * pairsPerDay).arg(pairsPerDay));
    m_chart->setData("Завантаженість аудиторій, %", labels, values, "%", 100.0);
}

void RoomLoadDialog::onShowScheduleClicked() {
    const auto *item = m_tableWidget->item(m_tableWidget->currentRow(), 0);
    if (!item) {
        QMessageBox::information(this, "Інформація", "Оберіть аудиторію у таблиці.");
        return;
    }

    EntityScheduleDialog dlg(ViewMode::Classroom, item->data(Qt::UserRole).toInt(), item->text(), this);
    dlg.exec();
}
