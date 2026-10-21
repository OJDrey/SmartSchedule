#include "mainwindow.h"
#include <QFileDialog>
#include <QPdfWriter>
#include <QPageSize>
#include <QPageLayout>
#include <QTextDocument>

namespace {
constexpr int IdsRole = Qt::UserRole;
constexpr int LabelsRole = Qt::UserRole + 1;
constexpr int DaysCount = 5;

ReferenceConfig makeConfig(const QString &title,
                           const QList<ReferenceField> &fields,
                           Entity entity,
                           std::function<QList<ReferenceRow>()> load,
                           std::function<bool(int, const QStringList &, QString &)> save)
{
    ReferenceConfig config;
    config.title = title;
    config.fields = fields;
    config.load = std::move(load);
    config.save = std::move(save);
    config.remove = [entity](int id, QString &error) {
        return DatabaseManager::instance().deleteEntity(entity, id, error);
    };
    config.usage = [entity](int id) {
        return DatabaseManager::instance().countLessons(entity, id);
    };
    return config;
}

ReferenceConfig groupsConfig()
{
    return makeConfig("Керування групами",
                      {{"Назва групи", ReferenceField::Text}},
                      Entity::Group,
                      [] {
                          QList<ReferenceRow> rows;
                          for (const auto &g : DatabaseManager::instance().getGroups())
                              rows.append({g.id, {g.name}});
                          return rows;
                      },
                      [](int id, const QStringList &v, QString &error) {
                          return DatabaseManager::instance().saveGroup(id, v[0], error);
                      });
}

ReferenceConfig subjectsConfig()
{
    return makeConfig("Керування предметами",
                      {{"Назва предмета", ReferenceField::Text}, {"Колір", ReferenceField::Color}},
                      Entity::Subject,
                      [] {
                          QList<ReferenceRow> rows;
                          for (const auto &s : DatabaseManager::instance().getSubjects())
                              rows.append({s.id, {s.name, s.colorHex}});
                          return rows;
                      },
                      [](int id, const QStringList &v, QString &error) {
                          return DatabaseManager::instance().saveSubject(id, v[0], v[1], error);
                      });
}

ReferenceConfig teachersConfig()
{
    return makeConfig("Керування викладачами",
                      {{"ПІБ викладача", ReferenceField::Text}, {"Кафедра", ReferenceField::Text}},
                      Entity::Teacher,
                      [] {
                          QList<ReferenceRow> rows;
                          for (const auto &t : DatabaseManager::instance().getTeachers())
                              rows.append({t.id, {t.name, t.department}});
                          return rows;
                      },
                      [](int id, const QStringList &v, QString &error) {
                          return DatabaseManager::instance().saveTeacher(id, v[0], v[1], error);
                      });
}

ReferenceConfig classroomsConfig()
{
    return makeConfig("Керування аудиторіями",
                      {{"Номер аудиторії", ReferenceField::Text}, {"Місткість", ReferenceField::Number}},
                      Entity::Classroom,
                      [] {
                          QList<ReferenceRow> rows;
                          for (const auto &c : DatabaseManager::instance().getClassrooms())
                              rows.append({c.id, {c.roomNumber, QString::number(c.capacity)}});
                          return rows;
                      },
                      [](int id, const QStringList &v, QString &error) {
                          return DatabaseManager::instance().saveClassroom(id, v[0], v[1].toInt(), error);
                      });
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    loadGroups();

    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateCurrentWeekInfo);
    timer->start(60 * 60 * 1000);
}

void MainWindow::setupUi() {
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto *mainLayout = new QVBoxLayout(centralWidget);

    QMenu *fileMenu = menuBar()->addMenu("📁 Файл");
    fileMenu->addAction("📄 Експорт розкладу у PDF...", this, &MainWindow::onExportPdf);
    fileMenu->addSeparator();
    fileMenu->addAction("🚪 Вийти", this, &QWidget::close);

    QMenu *dataMenu = menuBar()->addMenu("🛠️ Керування даними");
    addReferenceAction(dataMenu, "🎓 Групи...", groupsConfig);
    addReferenceAction(dataMenu, "📘 Предмети...", subjectsConfig);
    addReferenceAction(dataMenu, "👨‍🏫 Викладачі...", teachersConfig);
    addReferenceAction(dataMenu, "🚪 Аудиторії...", classroomsConfig);
    dataMenu->addSeparator();
    dataMenu->addAction("🔔 Розклад дзвінків...", this, &MainWindow::onOpenBellsDialog);

    QMenu *settingsMenu = menuBar()->addMenu("⚙️ Налаштування");
    settingsMenu->addAction("ℹ️ Про програму...", this, &MainWindow::onOpenAboutDialog);

    m_statusWeekLabel = new QLabel(this);
    m_statusWeekLabel->setStyleSheet("font-weight: bold; color: #1565C0; padding: 4px; background: #E3F2FD; border-radius: 4px;");
    updateCurrentWeekInfo();

    m_groupCombo = new QComboBox(this);

    m_weekFilterCombo = new QComboBox(this);
    m_weekFilterCombo->addItem("📅 Усі тижні (Повний розклад)", 0);
    for (int w = 1; w <= 4; ++w) {
        m_weekFilterCombo->addItem(weekTypeName(w), w);
    }

    m_addBtn = new QPushButton("➕ Додати пару", this);
    m_deleteBtn = new QPushButton("❌ Видалити обрану пару", this);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Група:", this));
    topLayout->addWidget(m_groupCombo, 1);
    topLayout->addWidget(new QLabel("Фільтр тижня:", this));
    topLayout->addWidget(m_weekFilterCombo);
    topLayout->addWidget(m_addBtn);
    topLayout->addWidget(m_deleteBtn);

    m_tableWidget = new QTableWidget(0, DaysCount, this);
    m_tableWidget->setHorizontalHeaderLabels({"Понеділок", "Вівторок", "Середа", "Четвер", "П'ятниця"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    mainLayout->addWidget(m_statusWeekLabel);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_tableWidget);

    connect(m_groupCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshScheduleTable);
    connect(m_weekFilterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshScheduleTable);
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddEntryClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEntryClicked);
}

void MainWindow::addReferenceAction(QMenu *menu, const QString &text, std::function<ReferenceConfig()> makeConfig) {
    menu->addAction(text, this, [this, makeConfig] {
        ReferenceDialog dlg(makeConfig(), this);
        dlg.exec();
        loadGroups();
    });
}

void MainWindow::updateCurrentWeekInfo() {
    const QDate today = QDate::currentDate();
    const bool isNumerator = today.weekNumber() % 2 == 1;

    m_statusWeekLabel->setText(QString("🗓️ Сьогодні: %1 (%2) | Поточний тиждень за 2-тижневим циклом: %3")
                                   .arg(today.toString("dd.MM.yyyy"), QLocale().toString(today, "dddd"),
                                        weekTypeName(isNumerator ? 1 : 2)));
}

void MainWindow::loadGroups() {
    {
        const QSignalBlocker blocker(m_groupCombo);
        const int previousId = m_groupCombo->currentData().toInt();

        m_groupCombo->clear();
        for (const auto &g : DatabaseManager::instance().getGroups()) {
            m_groupCombo->addItem(g.name, g.id);
        }

        const int index = m_groupCombo->findData(previousId);
        if (index >= 0) m_groupCombo->setCurrentIndex(index);
    }
    refreshScheduleTable();
}

void MainWindow::onOpenBellsDialog() {
    BellsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshScheduleTable();
    }
}

void MainWindow::onOpenAboutDialog() {
    QMessageBox::about(this, "Про програму SmartSchedule",
                       "<h2>SmartSchedule v10.0</h2>"
                       "<p><b>Автоматизація розкладу занять навчального закладу</b></p>"
                       "<p>Програма призначена для формування та керування навчальним розкладом, "
                       "контролю часових конфліктів викладачів, аудиторій та груп, "
                       "підтримки багатотижневих циклів занять (чисельник/знаменник), "
                       "гнучкого налаштування розкладу дзвінків та експорту розкладу у PDF.</p>"
                       "<hr>"
                       "<p><b>Технологічний стек:</b> C++17, Qt 6 (Qt Widgets), SQLite3, CMake</p>"
                       "<p><b>Курсова робота з дисципліни:</b> «Системне програмування»</p>");
}

void MainWindow::onExportPdf() {
    if (m_groupCombo->currentData().toInt() <= 0) {
        QMessageBox::warning(this, "Увага", "Будь ласка, оберіть групу!");
        return;
    }

    const QString path = QFileDialog::getSaveFileName(this, "Експорт розкладу",
                                                      m_groupCombo->currentText() + ".pdf", "PDF (*.pdf)");
    if (path.isEmpty()) return;

    auto toHtml = [](const QString &text) {
        return text.toHtmlEscaped().replace(kNewLine, "<br>");
    };

    QString html = QString("<h2 align='center'>Розклад групи %1</h2><p align='center'>%2</p>")
                       .arg(m_groupCombo->currentText().toHtmlEscaped(), m_weekFilterCombo->currentText().toHtmlEscaped());
    html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'><tr><th></th>";

    for (int c = 0; c < DaysCount; ++c) {
        html += "<th>" + toHtml(m_tableWidget->horizontalHeaderItem(c)->text()) + "</th>";
    }
    html += "</tr>";

    for (int r = 0; r < m_tableWidget->rowCount(); ++r) {
        html += "<tr><td><b>" + toHtml(m_tableWidget->verticalHeaderItem(r)->text()) + "</b></td>";
        for (int c = 0; c < DaysCount; ++c) {
            const auto *item = m_tableWidget->item(r, c);
            html += "<td align='center'>" + (item ? toHtml(item->text()) : QString()) + "</td>";
        }
        html += "</tr>";
    }
    html += "</table>";

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&writer);

    QMessageBox::information(this, "Експорт", "Розклад успішно збережено у PDF.");
}

void MainWindow::refreshScheduleTable() {
    m_tableWidget->clearContents();

    const auto bells = DatabaseManager::instance().getBellSchedule();
    m_tableWidget->setRowCount(bells.size());

    QStringList pairHeaders;
    for (const auto &b : bells) {
        pairHeaders.append(QString("%1 пара").arg(b.pairNumber) + kNewLine + QString("%1 - %2").arg(b.startTime, b.endTime));
    }
    m_tableWidget->setVerticalHeaderLabels(pairHeaders);

    const int groupId = m_groupCombo->currentData().toInt();
    if (groupId <= 0) return;

    const auto schedule = DatabaseManager::instance().getScheduleForGroup(groupId, m_weekFilterCombo->currentData().toInt());

    QMap<QPair<int, int>, QList<ScheduleRecord>> cellMap;
    for (const auto &rec : schedule) {
        cellMap[qMakePair(rec.pairNumber - 1, rec.dayOfWeek - 1)].append(rec);
    }

    const QString separator = QString(kNewLine) + "-----------------" + kNewLine;

    for (auto it = cellMap.constBegin(); it != cellMap.constEnd(); ++it) {
        const int row = it.key().first;
        const int col = it.key().second;
        const auto &records = it.value();

        if (row < 0 || row >= bells.size() || col < 0 || col >= DaysCount) continue;

        QStringList lines, labels;
        QVariantList ids;
        for (const auto &rec : records) {
            const QString prefix = rec.weekType > 0 ? QString("[%1 тижд] ").arg(rec.weekType) : QString();
            const QString title = prefix + rec.subjectName;
            lines << title + kNewLine + QString("Викл: %1 | Ауд: %2").arg(rec.teacherName, rec.roomNumber);
            labels << QString("%1 — %2, ауд. %3").arg(title, rec.teacherName, rec.roomNumber);
            ids << rec.id;
        }

        auto *item = new QTableWidgetItem(lines.join(separator));
        item->setTextAlignment(Qt::AlignCenter);

        QColor bgColor = records.size() > 1 ? QColor("#E3F2FD") : QColor(records.first().colorHex);
        if (!bgColor.isValid()) bgColor = QColor("#4CAF50");

        item->setBackground(bgColor);
        item->setForeground(bgColor.lightness() < 128 ? Qt::white : Qt::black);
        item->setData(IdsRole, ids);
        item->setData(LabelsRole, labels);

        m_tableWidget->setItem(row, col, item);
    }
}

void MainWindow::onAddEntryClicked() {
    const int groupId = m_groupCombo->currentData().toInt();
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
    const auto *item = m_tableWidget->currentItem();
    if (!item) {
        QMessageBox::information(this, "Видалення", "Будь ласка, оберіть комірку розкладу для видалення.");
        return;
    }

    const QVariantList ids = item->data(IdsRole).toList();
    if (ids.isEmpty()) {
        QMessageBox::information(this, "Видалення", "Обрана комірка порожня.");
        return;
    }

    int index = 0;
    if (ids.size() > 1) {
        const QStringList labels = item->data(LabelsRole).toStringList();
        bool ok = false;
        const QString chosen = QInputDialog::getItem(this, "Видалення", "Оберіть заняття для видалення:", labels, 0, false, &ok);
        if (!ok) return;
        index = qMax(0, labels.indexOf(chosen));
    }

    const auto answer = QMessageBox::question(this, "Підтвердження",
                                              "Ви дійсно бажаєте видалити обране заняття з розкладу?",
                                              QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    if (DatabaseManager::instance().deleteScheduleRecord(ids.at(index).toInt())) {
        refreshScheduleTable();
    } else {
        QMessageBox::critical(this, "Помилка", "Не вдалося видалити заняття.");
    }
}
