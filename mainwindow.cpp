#include "mainwindow.h"
#include "tablefile.h"
#include "theme.h"
#include <QActionGroup>
#include <QFileDialog>
#include <QPdfWriter>
#include <QPageSize>
#include <QPageLayout>
#include <QTextDocument>
#include <QPrinter>
#include <QPrintPreviewDialog>

namespace {
constexpr int IdsRole = Qt::UserRole;
constexpr int LabelsRole = Qt::UserRole + 1;
constexpr int DaysCount = 5;
const QStringList DayNames = {"Понеділок", "Вівторок", "Середа", "Четвер", "П'ятниця"};

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
                              rows.append(ReferenceRow{g.id, {g.name}});
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
                              rows.append(ReferenceRow{s.id, {s.name, s.colorHex}});
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
                              rows.append(ReferenceRow{t.id, {t.name, t.department}});
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
                              rows.append(ReferenceRow{c.id, {c.roomNumber, QString::number(c.capacity)}});
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
    connect(timer, &QTimer::timeout, this, [this] {
        updateCurrentWeekInfo();
        updateHighlight();
    });
    timer->start(60 * 1000);
}

void MainWindow::setupUi() {
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto *mainLayout = new QVBoxLayout(centralWidget);

    setupMenus();

    m_statusWeekLabel = new QLabel(this);
    m_statusWeekLabel->setStyleSheet("font-weight: bold; padding: 4px; border-radius: 4px; "
                                     "background: palette(highlight); color: palette(highlighted-text);");
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
    m_windowsCheck = new QCheckBox("🕳️ Показати вікна", this);
    m_windowsLabel = new QLabel(this);
    m_windowsLabel->setStyleSheet("font-weight: bold;");

    topLayout->addWidget(m_weekFilterCombo);
    topLayout->addWidget(m_windowsCheck);
    topLayout->addWidget(m_windowsLabel);
    topLayout->addWidget(m_addBtn);
    topLayout->addWidget(m_deleteBtn);

    m_tableWidget = new QTableWidget(0, DaysCount, this);
    m_tableWidget->setHorizontalHeaderLabels(DayNames);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    m_delegate = new HighlightDelegate(m_tableWidget);
    m_tableWidget->setItemDelegate(m_delegate);

    mainLayout->addWidget(m_statusWeekLabel);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_tableWidget);

    connect(m_groupCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshScheduleTable);
    connect(m_weekFilterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshScheduleTable);
    connect(m_windowsCheck, &QCheckBox::toggled, this, &MainWindow::refreshScheduleTable);
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddEntryClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEntryClicked);
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &MainWindow::onCellDoubleClicked);
}

void MainWindow::setupMenus() {
    QMenu *fileMenu = menuBar()->addMenu("📁 Файл");
    fileMenu->addAction("📄 Експорт у PDF...", this, &MainWindow::onExportPdf);
    fileMenu->addAction("📊 Експорт в Excel / CSV...", this, &MainWindow::onExportTable);
    QAction *printAction = fileMenu->addAction("🖨️ Друк / попередній перегляд...", this, &MainWindow::onPrint);
    printAction->setShortcut(QKeySequence::Print);
    fileMenu->addSeparator();
    fileMenu->addAction("💾 Резервна копія бази даних...", this, &MainWindow::onBackupDatabase);
    fileMenu->addAction("♻️ Відновити базу з копії...", this, &MainWindow::onRestoreDatabase);
    fileMenu->addSeparator();
    fileMenu->addAction("🚪 Вийти", this, &QWidget::close);

    QMenu *dataMenu = menuBar()->addMenu("🛠️ Керування даними");
    addReferenceAction(dataMenu, "🎓 Групи...", groupsConfig);
    addReferenceAction(dataMenu, "📘 Предмети...", subjectsConfig);
    addReferenceAction(dataMenu, "👨‍🏫 Викладачі...", teachersConfig);
    addReferenceAction(dataMenu, "🚪 Аудиторії...", classroomsConfig);
    dataMenu->addSeparator();
    dataMenu->addAction("🔔 Розклад дзвінків...", this, &MainWindow::onOpenBellsDialog);

    QMenu *reportsMenu = menuBar()->addMenu("📈 Звіти");
    reportsMenu->addAction("👨‍🏫 Навантаження викладачів...", this, &MainWindow::onShowWorkload);

    QMenu *settingsMenu = menuBar()->addMenu("⚙️ Налаштування");
    QMenu *themeMenu = settingsMenu->addMenu("🎨 Тема оформлення");
    auto *themeGroup = new QActionGroup(this);

    struct ThemeEntry {
        const char *text;
        Theme theme;
    };
    const ThemeEntry entries[] = {
        {"☀️ Світла", Theme::Light},
        {"🌙 Темна", Theme::Dark},
        {"🖥️ Системна", Theme::System}
    };

    for (const auto &entry : entries) {
        QAction *action = themeMenu->addAction(QString::fromUtf8(entry.text));
        action->setCheckable(true);
        action->setActionGroup(themeGroup);
        action->setChecked(entry.theme == ThemeManager::saved());
        connect(action, &QAction::triggered, this, [theme = entry.theme] {
            ThemeManager::save(theme);
            ThemeManager::apply(theme);
        });
    }

    settingsMenu->addAction("ℹ️ Про програму...", this, &MainWindow::onOpenAboutDialog);
}

void MainWindow::addReferenceAction(QMenu *menu, const QString &text, std::function<ReferenceConfig()> makeConfig) {
    menu->addAction(text, this, [this, makeConfig] {
        ReferenceDialog dlg(makeConfig(), this);
        dlg.exec();
        loadGroups();
    });
}

bool MainWindow::ensureGroupSelected() {
    if (m_groupCombo->currentData().toInt() > 0) return true;
    QMessageBox::warning(this, "Увага", "Будь ласка, оберіть групу!");
    return false;
}

void MainWindow::updateCurrentWeekInfo() {
    const QDate today = QDate::currentDate();
    const bool isNumerator = today.weekNumber() % 2 == 1;

    m_statusWeekLabel->setText(QString("🗓️ Сьогодні: %1 (%2) | Поточний тиждень за 2-тижневим циклом: %3")
                                   .arg(today.toString("dd.MM.yyyy"), QLocale().toString(today, "dddd"),
                                        weekTypeName(isNumerator ? 1 : 2)));
}

void MainWindow::updateHighlight() {
    const int dayOfWeek = QDate::currentDate().dayOfWeek();
    const int todayColumn = dayOfWeek <= DaysCount ? dayOfWeek - 1 : -1;

    int currentRow = -1;
    if (todayColumn >= 0) {
        const QTime now = QTime::currentTime();
        for (int i = 0; i < m_bells.size(); ++i) {
            const QTime start = QTime::fromString(m_bells[i].startTime, "hh:mm");
            const QTime end = QTime::fromString(m_bells[i].endTime, "hh:mm");
            if (now >= start && now <= end) {
                currentRow = i;
                break;
            }
        }
    }

    QStringList days = DayNames;
    if (todayColumn >= 0) days[todayColumn] = "▶ " + days[todayColumn] + " (сьогодні)";
    m_tableWidget->setHorizontalHeaderLabels(days);

    QStringList pairs = m_pairHeaders;
    if (currentRow >= 0 && currentRow < pairs.size()) pairs[currentRow] = "▶ " + pairs[currentRow];
    m_tableWidget->setVerticalHeaderLabels(pairs);

    m_delegate->todayColumn = todayColumn;
    m_delegate->currentRow = currentRow;
    m_tableWidget->viewport()->update();
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
                       "<h2>SmartSchedule v14.0</h2>"
                       "<p><b>Автоматизація розкладу занять навчального закладу</b></p>"
                       "<p>Програма призначена для формування та керування навчальним розкладом, "
                       "контролю часових конфліктів викладачів, аудиторій та груп, "
                       "підтримки багатотижневих циклів занять (чисельник/знаменник), "
                       "підбору вільних аудиторій, гнучкого налаштування розкладу дзвінків, "
                       "експорту у PDF, Excel (.xlsx) та CSV, імпорту довідників, друку розкладу, аналізу навантаження викладачів, пошуку «вікон» та резервного копіювання бази даних.</p>"
                       "<hr>"
                       "<p><b>Технологічний стек:</b> C++17, Qt 6 (Qt Widgets), SQLite3, CMake</p>"
                       "<p><b>Курсова робота з дисципліни:</b> «Системне програмування»</p>");
}

QString MainWindow::buildScheduleHtml() const {
    auto toHtml = [](const QString &text) {
        return text.toHtmlEscaped().replace(kNewLine, "<br>");
    };

    QString html = QString("<h2 align='center'>Розклад групи %1</h2><p align='center'>%2</p>")
                       .arg(m_groupCombo->currentText().toHtmlEscaped(), m_weekFilterCombo->currentText().toHtmlEscaped());
    html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'><tr><th></th>";

    for (const auto &day : DayNames) {
        html += "<th>" + toHtml(day) + "</th>";
    }
    html += "</tr>";

    for (int r = 0; r < m_tableWidget->rowCount(); ++r) {
        html += "<tr><td><b>" + toHtml(m_pairHeaders.value(r)) + "</b></td>";
        for (int c = 0; c < DaysCount; ++c) {
            const auto *item = m_tableWidget->item(r, c);
            html += "<td align='center'>" + (item ? toHtml(item->text()) : QString()) + "</td>";
        }
        html += "</tr>";
    }
    html += "</table>";
    return html;
}

void MainWindow::onExportPdf() {
    if (!ensureGroupSelected()) return;

    const QString path = QFileDialog::getSaveFileName(this, "Експорт розкладу",
                                                      m_groupCombo->currentText() + ".pdf", "PDF (*.pdf)");
    if (path.isEmpty()) return;

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setHtml(buildScheduleHtml());
    doc.print(&writer);

    QMessageBox::information(this, "Експорт", "Розклад успішно збережено у PDF.");
}

QList<QStringList> MainWindow::scheduleRows() const {
    QList<QStringList> rows;
    rows.append(QStringList{"Група", m_groupCombo->currentText(), m_weekFilterCombo->currentText()});
    rows.append(QStringList());

    QStringList header{"Пара / час"};
    header += DayNames;
    rows.append(header);

    for (int r = 0; r < m_tableWidget->rowCount(); ++r) {
        QString pair = m_pairHeaders.value(r);
        pair.replace(kNewLine, " ");

        QStringList row{pair};
        for (int c = 0; c < DaysCount; ++c) {
            const auto *item = m_tableWidget->item(r, c);
            row << (item ? item->text() : QString());
        }
        rows.append(row);
    }
    return rows;
}

bool MainWindow::saveScheduleXlsx(const QString &path) const {
    QXlsx::Document doc;

    QXlsx::Format titleFormat;
    titleFormat.setFontBold(true);
    titleFormat.setFontSize(14);
    titleFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);

    const QString title = QString("Розклад групи %1 (%2)").arg(m_groupCombo->currentText(), m_weekFilterCombo->currentText());
    doc.write(1, 1, title, titleFormat);
    doc.mergeCells(QXlsx::CellRange(1, 1, 1, DaysCount + 1), titleFormat);

    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setTextWrap(true);
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    headerFormat.setBorderStyle(QXlsx::Format::BorderThin);
    headerFormat.setFillPattern(QXlsx::Format::PatternSolid);
    headerFormat.setPatternBackgroundColor(QColor("#BBDEFB"));

    doc.write(3, 1, QString("Пара / час"), headerFormat);
    for (int c = 0; c < DaysCount; ++c) {
        doc.write(3, c + 2, DayNames.at(c), headerFormat);
    }

    for (int r = 0; r < m_tableWidget->rowCount(); ++r) {
        const int excelRow = r + 4;
        doc.write(excelRow, 1, m_pairHeaders.value(r), headerFormat);

        for (int c = 0; c < DaysCount; ++c) {
            QXlsx::Format cellFormat;
            cellFormat.setTextWrap(true);
            cellFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
            cellFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
            cellFormat.setBorderStyle(QXlsx::Format::BorderThin);

            QString text;
            if (const auto *item = m_tableWidget->item(r, c)) {
                text = item->text();
                cellFormat.setFillPattern(QXlsx::Format::PatternSolid);
                cellFormat.setPatternBackgroundColor(item->background().color());
                cellFormat.setFontColor(item->foreground().color());
            }
            doc.write(excelRow, c + 2, text, cellFormat);
        }
        doc.setRowHeight(excelRow, 64);
    }

    doc.setColumnWidth(1, 14);
    doc.setColumnWidth(2, DaysCount + 1, 34);
    return doc.saveAs(path);
}

void MainWindow::onExportTable() {
    if (!ensureGroupSelected()) return;

    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(this, "Експорт розкладу", m_groupCombo->currentText() + ".xlsx",
                                                "Excel (*.xlsx);;CSV (*.csv)", &selectedFilter);
    if (path.isEmpty()) return;
    if (!TableFile::hasKnownSuffix(path)) path += selectedFilter.contains("csv") ? ".csv" : ".xlsx";

    QString error;
    bool ok = false;
    if (TableFile::isXlsx(path)) {
        ok = saveScheduleXlsx(path);
        if (!ok) error = "не вдалося записати файл Excel.";
    } else {
        ok = Csv::write(path, scheduleRows(), error);
    }

    if (ok) {
        QMessageBox::information(this, "Експорт", "Розклад успішно збережено.");
    } else {
        QMessageBox::critical(this, "Помилка", "Не вдалося зберегти файл: " + error);
    }
}

void MainWindow::onShowWorkload() {
    WorkloadDialog dlg(this);
    dlg.exec();
}

void MainWindow::onBackupDatabase() {
    const QString defaultName = QString("schedule_backup_%1.db")
    .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));
    const QString path = QFileDialog::getSaveFileName(this, "Резервна копія бази даних", defaultName,
                                                      "База SmartSchedule (*.db)");
    if (path.isEmpty()) return;

    QString error;
    if (DatabaseManager::instance().backupTo(path, error)) {
        QMessageBox::information(this, "Резервна копія", "Копію бази даних успішно збережено.");
    } else {
        QMessageBox::critical(this, "Помилка", error);
    }
}

void MainWindow::onRestoreDatabase() {
    const QString path = QFileDialog::getOpenFileName(this, "Відновлення з копії", QString(),
                                                      "База SmartSchedule (*.db *.sqlite)");
    if (path.isEmpty()) return;

    const QString warning = QString("Усі поточні дані буде замінено даними з копії.") + kNewLine
                            + "Поточну базу буде збережено поруч у файлі з розширенням .before-restore."
                            + kNewLine + kNewLine + "Продовжити?";
    if (QMessageBox::warning(this, "Підтвердження відновлення", warning,
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    QString error;
    const bool ok = DatabaseManager::instance().restoreFrom(path, error);
    loadGroups();

    if (ok) {
        QMessageBox::information(this, "Відновлення", "Базу даних успішно відновлено.");
    } else {
        QMessageBox::critical(this, "Помилка", error);
    }
}

void MainWindow::onPrint() {
    if (!ensureGroupSelected()) return;

    QTextDocument doc;
    doc.setHtml(buildScheduleHtml());

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageOrientation(QPageLayout::Landscape);

    QPrintPreviewDialog preview(&printer, this);
    preview.resize(1000, 700);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, [&doc](QPrinter *p) {
        doc.print(p);
    });
    preview.exec();
}

void MainWindow::refreshScheduleTable() {
    m_tableWidget->clearContents();

    m_bells = DatabaseManager::instance().getBellSchedule();
    m_tableWidget->setRowCount(m_bells.size());

    m_pairHeaders.clear();
    for (const auto &b : m_bells) {
        m_pairHeaders.append(QString("%1 пара").arg(b.pairNumber) + kNewLine + QString("%1 - %2").arg(b.startTime, b.endTime));
    }
    updateHighlight();

    m_delegate->windows.clear();
    m_windowsLabel->clear();

    const int groupId = m_groupCombo->currentData().toInt();
    if (groupId <= 0) return;

    const int weekFilter = m_weekFilterCombo->currentData().toInt();
    const auto schedule = DatabaseManager::instance().getScheduleForGroup(groupId, weekFilter);

    if (m_windowsCheck->isChecked()) {
        const auto windows = ScheduleAnalyzer::findWindows(schedule, weekFilter);
        for (const auto &w : windows) {
            m_delegate->windows.insert(WindowsDelegate::key(w.pair - 1, w.day - 1));
        }
        m_windowsLabel->setText(QString("Вікон: %1").arg(windows.size()));
        m_tableWidget->viewport()->update();
    }

    QMap<QPair<int, int>, QList<ScheduleRecord>> cellMap;
    for (const auto &rec : schedule) {
        cellMap[qMakePair(rec.pairNumber - 1, rec.dayOfWeek - 1)].append(rec);
    }

    const QString separator = QString(kNewLine) + "-----------------" + kNewLine;

    for (auto it = cellMap.constBegin(); it != cellMap.constEnd(); ++it) {
        const int row = it.key().first;
        const int col = it.key().second;
        const auto &records = it.value();

        if (row < 0 || row >= m_bells.size() || col < 0 || col >= DaysCount) continue;

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
    if (!ensureGroupSelected()) return;

    AddEntryDialog dlg(m_groupCombo->currentData().toInt(), this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshScheduleTable();
    }
}

int MainWindow::pickRecordId(const QTableWidgetItem *item, const QString &title, const QString &prompt) {
    const QVariantList ids = item->data(IdsRole).toList();
    if (ids.isEmpty()) return 0;
    if (ids.size() == 1) return ids.first().toInt();

    const QStringList labels = item->data(LabelsRole).toStringList();
    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, title, prompt, labels, 0, false, &ok);
    if (!ok) return 0;
    return ids.at(qMax(0, labels.indexOf(chosen))).toInt();
}

void MainWindow::onCellDoubleClicked(int row, int column) {
    const int groupId = m_groupCombo->currentData().toInt();
    if (groupId <= 0) return;

    int recordId = 0;
    if (const auto *item = m_tableWidget->item(row, column)) {
        recordId = pickRecordId(item, "Редагування", "Оберіть заняття для редагування:");
        if (recordId <= 0) return;
    }

    const int pairNumber = row < m_bells.size() ? m_bells[row].pairNumber : 0;
    AddEntryDialog dlg(groupId, this, recordId, column + 1, pairNumber);
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

    const int recordId = pickRecordId(item, "Видалення", "Оберіть заняття для видалення:");
    if (recordId <= 0) return;

    const auto answer = QMessageBox::question(this, "Підтвердження",
                                              "Ви дійсно бажаєте видалити обране заняття з розкладу?",
                                              QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    if (DatabaseManager::instance().deleteScheduleRecord(recordId)) {
        refreshScheduleTable();
    } else {
        QMessageBox::critical(this, "Помилка", "Не вдалося видалити заняття.");
    }
}