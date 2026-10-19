#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    loadGroups();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Верхнє меню програми
    QMenu *settingsMenu = menuBar()->addMenu("⚙️ Налаштування");
    QAction *bellsAction = settingsMenu->addAction("🔔 Розклад дзвінків (години пар)...");
    connect(bellsAction, &QAction::triggered, this, &MainWindow::onOpenBellsDialog);

    // Статусний рядок інформації про поточний тиждень
    m_statusWeekLabel = new QLabel(this);
    m_statusWeekLabel->setStyleSheet("font-weight: bold; color: #1565C0; padding: 4px; background: #E3F2FD; border-radius: 4px;");
    updateCurrentWeekInfo();

    // Панель фільтрів
    QHBoxLayout *topLayout = new QHBoxLayout();

    QLabel *groupLabel = new QLabel("Група:", this);
    m_groupCombo = new QComboBox(this);

    QLabel *weekLabel = new QLabel("Фільтр тижня:", this);
    m_weekFilterCombo = new QComboBox(this);
    m_weekFilterCombo->addItem("📅 Усі тижні (Повний розклад)", 0);
    m_weekFilterCombo->addItem("1-й тиждень (Чисельник)", 1);
    m_weekFilterCombo->addItem("2-й тиждень (Знаменник)", 2);
    m_weekFilterCombo->addItem("3-й тиждень", 3);
    m_weekFilterCombo->addItem("4-й тиждень", 4);

    m_addBtn = new QPushButton("➕ Додати пару", this);
    m_deleteBtn = new QPushButton("❌ Видалити обрану пару", this);

    topLayout->addWidget(groupLabel);
    topLayout->addWidget(m_groupCombo, 1);
    topLayout->addWidget(weekLabel);
    topLayout->addWidget(m_weekFilterCombo);
    topLayout->addWidget(m_addBtn);
    topLayout->addWidget(m_deleteBtn);

    // Таблиця сітки розкладу
    m_tableWidget = new QTableWidget(0, 5, this);
    QStringList days = {"Понеділок", "Вівторок", "Середа", "Четвер", "П'ятниця"};
    m_tableWidget->setHorizontalHeaderLabels(days);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mainLayout->addWidget(m_statusWeekLabel);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_tableWidget);

    connect(m_groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onGroupChanged);
    connect(m_weekFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onWeekFilterChanged);
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddEntryClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEntryClicked);
}

void MainWindow::updateCurrentWeekInfo() {
    QDate today = QDate::currentDate();
    int weekNum = today.weekNumber();
    int cycle2Week = (weekNum % 2 == 1) ? 1 : 2;

    m_statusWeekLabel->setText(QString("🗓️ Сьогодні: %1 (%2) | Поточний тиждень за 2-тижневим циклом: %3")
                                   .arg(today.toString("dd.MM.yyyy"))
                                   .arg(today.toString("dddd"))
                                   .arg(cycle2Week == 1 ? "1-й тиждень (Чисельник)" : "2-й тиждень (Знаменник)"));
}

void MainWindow::loadGroups() {
    m_groupCombo->clear();
    auto groups = DatabaseManager::instance().getGroups();
    for (const auto &g : groups) {
        m_groupCombo->addItem(g.name, g.id);
    }
    refreshScheduleTable();
}

void MainWindow::onGroupChanged(int index) {
    Q_UNUSED(index);
    refreshScheduleTable();
}

void MainWindow::onWeekFilterChanged(int index) {
    Q_UNUSED(index);
    refreshScheduleTable();
}

void MainWindow::onOpenBellsDialog() {
    BellsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshScheduleTable();
    }
}

void MainWindow::refreshScheduleTable() {
    m_tableWidget->clearContents();

    auto bells = DatabaseManager::instance().getBellSchedule();
    m_tableWidget->setRowCount(bells.size());

    QStringList pairHeaders;
    for (const auto &b : bells) {
        pairHeaders.append(QString("%1 пара\n%2 - %3").arg(b.pairNumber).arg(b.startTime).arg(b.endTime));
    }
    m_tableWidget->setVerticalHeaderLabels(pairHeaders);

    int groupId = m_groupCombo->currentData().toInt();
    int selectedWeekFilter = m_weekFilterCombo->currentData().toInt();

    if (groupId <= 0) return;

    auto schedule = DatabaseManager::instance().getScheduleForGroup(groupId, selectedWeekFilter);

    QMap<QPair<int, int>, QList<ScheduleRecord>> cellMap;
    for (const auto &rec : schedule) {
        int col = rec.dayOfWeek - 1;
        int row = rec.pairNumber - 1;
        cellMap[qMakePair(row, col)].append(rec);
    }

    for (auto it = cellMap.constBegin(); it != cellMap.constEnd(); ++it) {
        int row = it.key().first;
        int col = it.key().second;
        const auto &records = it.value();

        if (row < 0 || row >= bells.size() || col < 0 || col >= 5) continue;

        QStringList cellLines;
        QString mainColorHex = "#4CAF50";

        for (const auto &rec : records) {
            mainColorHex = rec.colorHex;

            QString weekPrefix = "";
            if (rec.weekType == 1) weekPrefix = "[1 тижд] ";
            else if (rec.weekType == 2) weekPrefix = "[2 тижд] ";
            else if (rec.weekType == 3) weekPrefix = "[3 тижд] ";
            else if (rec.weekType == 4) weekPrefix = "[4 тижд] ";

            cellLines.append(QString("%1%2\nВикл: %3 | Ауд: %4")
                                 .arg(weekPrefix)
                                 .arg(rec.subjectName)
                                 .arg(rec.teacherName)
                                 .arg(rec.roomNumber));
        }

        QString fullCellText = cellLines.join("\n-----------------\n");
        QTableWidgetItem *item = new QTableWidgetItem(fullCellText);
        item->setTextAlignment(Qt::AlignCenter);

        QColor bgColor(mainColorHex);
        if (records.size() > 1) {
            bgColor = QColor("#E3F2FD");
        }

        item->setBackground(QBrush(bgColor));
        if (bgColor.lightness() < 128) {
            item->setForeground(QBrush(Qt::white));
        } else {
            item->setForeground(QBrush(Qt::black));
        }

        if (!records.isEmpty()) {
            item->setData(Qt::UserRole, records.first().id);
        }

        m_tableWidget->setItem(row, col, item);
    }
}

void MainWindow::onAddEntryClicked() {
    int groupId = m_groupCombo->currentData().toInt();
    if (groupId <= 0) {
        QMessageBox::warning(this, "Увага", "Будь ласка, оберіть групу!");
        return;
    }

    AddEntryDialog dlg(groupId, this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshScheduleTable();
    }
}

void MainWindow::onDeleteEntryClicked() {
    auto currentItem = m_tableWidget->currentItem();
    if (!currentItem) {
        QMessageBox::information(this, "Видалення", "Будь ласка, оберіть комірку розкладу для видалення.");
        return;
    }

    int recordId = currentItem->data(Qt::UserRole).toInt();
    if (recordId <= 0) {
        QMessageBox::information(this, "Видалення", "Обрана комірка порожня.");
        return;
    }

    auto res = QMessageBox::question(this, "Підтвердження", "Ви дійсно бажаєте видалити обране заняття з розкладу?", QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
        if (DatabaseManager::instance().deleteScheduleRecord(recordId)) {
            refreshScheduleTable();
        }
    }
}