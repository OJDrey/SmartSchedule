#include "mainwindow.h"
#include "tablefile.h"
#include "theme.h"
#include "schedulehtml.h"
#include "scheduleimporter.h"
#include "importscheduledialog.h"
#include "conflictreportdialog.h"
#include "roomloaddialog.h"
#include "workloaddialog.h"
#include "version.h"
#include <QActionGroup>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintPreviewDialog>

namespace {
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
    loadEntities(0);

    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] {
        updateCurrentWeekInfo();
        m_grid->refreshHighlight();
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

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("🎓 Група", 0);
    m_modeCombo->addItem("👨‍🏫 Викладач", 1);
    m_modeCombo->addItem("🚪 Аудиторія", 2);

    m_entityLabel = new QLabel("Група:", this);
    m_entityCombo = new QComboBox(this);

    m_weekFilterCombo = new QComboBox(this);
    m_weekFilterCombo->addItem("📅 Усі тижні (Повний розклад)", 0);
    for (int w = 1; w <= 4; ++w) {
        m_weekFilterCombo->addItem(weekTypeName(w), w);
    }

    m_windowsCheck = new QCheckBox("🕳️ Показати вікна", this);
    m_windowsLabel = new QLabel(this);
    m_windowsLabel->setStyleSheet("font-weight: bold;");

    m_addBtn = new QPushButton("➕ Додати пару", this);
    m_deleteBtn = new QPushButton("❌ Видалити пару", this);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Перегляд:", this));
    topLayout->addWidget(m_modeCombo);
    topLayout->addWidget(m_entityLabel);
    topLayout->addWidget(m_entityCombo, 1);
    topLayout->addWidget(new QLabel("Тиждень:", this));
    topLayout->addWidget(m_weekFilterCombo);
    topLayout->addWidget(m_windowsCheck);
    topLayout->addWidget(m_windowsLabel);
    topLayout->addWidget(m_addBtn);
    topLayout->addWidget(m_deleteBtn);

    m_filterBar = new FilterBar(this);
    m_grid = new ScheduleGrid(this);

    mainLayout->addWidget(m_statusWeekLabel);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_filterBar);
    mainLayout->addWidget(m_grid);

    connect(m_modeCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onModeChanged);
    connect(m_entityCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshScheduleTable);
    connect(m_weekFilterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshScheduleTable);
    connect(m_windowsCheck, &QCheckBox::toggled, this, &MainWindow::refreshScheduleTable);
    connect(m_filterBar, &FilterBar::filterChanged, this, &MainWindow::refreshScheduleTable);
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddEntryClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEntryClicked);
    connect(m_grid, &QTableWidget::cellDoubleClicked, this, &MainWindow::onCellDoubleClicked);
}

void MainWindow::setupMenus() {
    QMenu *fileMenu = menuBar()->addMenu("📁 Файл");
    fileMenu->addAction("📄 Експорт поточного розкладу у PDF...", this, &MainWindow::onExportPdf);
    fileMenu->addAction("📚 Експорт розкладів усіх груп в один PDF...", this, &MainWindow::onExportAllGroupsPdf);
    fileMenu->addAction("📊 Експорт поточного розкладу в Excel / CSV...", this, &MainWindow::onExportTable);
    fileMenu->addAction("🗃️ Експорт розкладу всіх груп (дані для імпорту)...", this, &MainWindow::onExportAllData);
    fileMenu->addAction("📥 Імпорт розкладу з Excel / CSV...", this, &MainWindow::onImportSchedule);
    fileMenu->addSeparator();
    QAction *printAction = fileMenu->addAction("🖨️ Друк поточного розкладу...", this, &MainWindow::onPrint);
    printAction->setShortcut(QKeySequence::Print);
    fileMenu->addAction("🖨️ Друк розкладів усіх груп...", this, &MainWindow::onPrintAllGroups);
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
    reportsMenu->addAction("🚪 Завантаженість аудиторій...", this, &MainWindow::onShowRoomLoad);
    reportsMenu->addAction("⚠️ Перевірка конфліктів розкладу...", this, &MainWindow::onShowConflicts);

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
        loadEntities(currentEntityId());
    });
}

ViewMode MainWindow::currentMode() const {
    return static_cast<ViewMode>(m_modeCombo->currentData().toInt());
}

int MainWindow::currentEntityId() const {
    return m_entityCombo->currentData().toInt();
}

bool MainWindow::ensureEntitySelected() {
    if (currentEntityId() > 0) return true;
    QMessageBox::warning(this, "Увага", "Список порожній або нічого не вибрано. Додайте записи у меню «Керування даними».");
    return false;
}

void MainWindow::onModeChanged() {
    m_lastIds[m_previousMode] = currentEntityId();
    m_previousMode = m_modeCombo->currentIndex();
    loadEntities(m_lastIds[m_previousMode]);
}

void MainWindow::loadEntities(int preferredId) {
    const ViewMode mode = currentMode();
    auto &db = DatabaseManager::instance();

    {
        const QSignalBlocker blocker(m_entityCombo);
        m_entityCombo->clear();

        switch (mode) {
        case ViewMode::Group:
            m_entityLabel->setText("Група:");
            for (const auto &g : db.getGroups()) m_entityCombo->addItem(g.name, g.id);
            break;
        case ViewMode::Teacher:
            m_entityLabel->setText("Викладач:");
            for (const auto &t : db.getTeachers()) m_entityCombo->addItem(t.name, t.id);
            break;
        case ViewMode::Classroom:
            m_entityLabel->setText("Аудиторія:");
            for (const auto &r : db.getClassrooms()) m_entityCombo->addItem(QString("Ауд. %1").arg(r.roomNumber), r.id);
            break;
        }

        const int index = m_entityCombo->findData(preferredId);
        if (index >= 0) m_entityCombo->setCurrentIndex(index);
    }

    m_windowsCheck->setEnabled(mode != ViewMode::Classroom);
    m_filterBar->reloadLists();
    refreshScheduleTable();
}

void MainWindow::showGroup(int groupId) {
    m_lastIds[0] = groupId;
    if (m_modeCombo->currentIndex() != 0) {
        m_modeCombo->setCurrentIndex(0);
    } else {
        loadEntities(groupId);
    }
}

void MainWindow::updateCurrentWeekInfo() {
    const QDate today = QDate::currentDate();
    const bool isNumerator = today.weekNumber() % 2 == 1;

    m_statusWeekLabel->setText(QString("🗓️ Сьогодні: %1 (%2) | Поточний тиждень за 2-тижневим циклом: %3")
                                   .arg(today.toString("dd.MM.yyyy"), QLocale().toString(today, "dddd"),
                                        weekTypeName(isNumerator ? 1 : 2)));
}

void MainWindow::refreshScheduleTable() {
    const ViewMode mode = currentMode();
    const int id = currentEntityId();
    const int week = m_weekFilterCombo->currentData().toInt();

    m_bells = DatabaseManager::instance().getBellSchedule();
    m_grid->setBells(m_bells);

    m_records = id > 0 ? DatabaseManager::instance().getScheduleForEntity(entityOf(mode), id, week)
                       : QList<ScheduleRecord>();

    const bool windows = m_windowsCheck->isChecked() && mode != ViewMode::Classroom;
    m_grid->showRecords(m_records, mode, m_filterBar->filter(), windows, week);
    m_windowsLabel->setText(windows ? QString("Вікон: %1").arg(m_grid->windowsCount()) : QString());
}

int MainWindow::pickRecordId(int row, int column, const QString &title, const QString &prompt) {
    const QVariantList ids = m_grid->recordIdsAt(row, column);
    if (ids.isEmpty()) return 0;
    if (ids.size() == 1) return ids.first().toInt();

    const QStringList labels = m_grid->recordLabelsAt(row, column);
    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, title, prompt, labels, 0, false, &ok);
    if (!ok) return 0;
    return ids.at(qMax(0, labels.indexOf(chosen))).toInt();
}

void MainWindow::openEntryDialog(int recordId, int day, int pair) {
    const ViewMode mode = currentMode();
    const int id = currentEntityId();

    EntryPreset preset;
    preset.day = day;
    preset.pair = pair;
    if (mode == ViewMode::Teacher) preset.teacherId = id;
    if (mode == ViewMode::Classroom) preset.classroomId = id;

    AddEntryDialog dlg(mode == ViewMode::Group ? id : 0, this, recordId, preset);
    if (dlg.exec() == QDialog::Accepted) {
        refreshScheduleTable();
    }
}

void MainWindow::onAddEntryClicked() {
    if (!ensureEntitySelected()) return;
    openEntryDialog(0, 0, 0);
}

void MainWindow::onCellDoubleClicked(int row, int column) {
    if (currentEntityId() <= 0) return;

    int recordId = 0;
    if (!m_grid->recordIdsAt(row, column).isEmpty()) {
        recordId = pickRecordId(row, column, "Редагування", "Оберіть заняття для редагування:");
        if (recordId <= 0) return;
    }
    openEntryDialog(recordId, column + 1, m_grid->pairNumberAt(row));
}

void MainWindow::onDeleteEntryClicked() {
    const int row = m_grid->currentRow();
    const int column = m_grid->currentColumn();
    if (row < 0 || column < 0 || m_grid->recordIdsAt(row, column).isEmpty()) {
        QMessageBox::information(this, "Видалення", "Будь ласка, оберіть комірку розкладу із заняттям для видалення.");
        return;
    }

    const int recordId = pickRecordId(row, column, "Видалення", "Оберіть заняття для видалення:");
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

void MainWindow::onOpenBellsDialog() {
    BellsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshScheduleTable();
    }
}

void MainWindow::onOpenAboutDialog() {
    QMessageBox::about(this, "Про програму SmartSchedule",
                       QString("<h2>SmartSchedule v%1</h2>"
                               "<p><b>Автоматизація розкладу занять навчального закладу</b></p>"
                               "<p>Формування та керування розкладом, контроль конфліктів викладачів, аудиторій та груп, "
                               "перегляд розкладу за групою, викладачем чи аудиторією, пошук і фільтри, "
                               "підбір вільних аудиторій, пошук «вікон», аналіз навантаження викладачів та завантаженості аудиторій, "
                               "звіт про конфлікти, експорт у PDF/Excel/CSV, імпорт розкладу з Excel, "
                               "друк та резервне копіювання бази даних.</p>"
                               "<hr>"
                               "<p><b>Технологічний стек:</b> C++17, Qt 6 (Qt Widgets), SQLite3, QXlsx, CMake</p>"
                               "<p><b>Курсова робота з дисципліни:</b> «Системне програмування»</p>")
                           .arg(APP_VERSION));
}

QString MainWindow::weekFilterTitle() const {
    const int week = m_weekFilterCombo->currentData().toInt();
    return week == 0 ? QString("Усі тижні (повний розклад)") : weekTypeName(week);
}

QString MainWindow::currentHtml() const {
    return ScheduleHtml::page(viewModeTitle(currentMode()) + " " + m_entityCombo->currentText(),
                              weekFilterTitle(), m_bells, m_records, currentMode());
}

QString MainWindow::allGroupsHtml() const {
    auto &db = DatabaseManager::instance();
    const int week = m_weekFilterCombo->currentData().toInt();
    const auto bells = db.getBellSchedule();

    QStringList pages;
    for (const auto &g : db.getGroups()) {
        pages << ScheduleHtml::page(viewModeTitle(ViewMode::Group) + " " + g.name, weekFilterTitle(), bells,
                                    db.getScheduleForGroup(g.id, week), ViewMode::Group, !pages.isEmpty());
    }
    return pages.isEmpty() ? QString() : ScheduleHtml::document(pages);
}

void MainWindow::showPrintPreview(const QString &html) {
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageOrientation(QPageLayout::Landscape);

    QPrintPreviewDialog preview(&printer, this);
    preview.resize(1000, 700);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, [&html](QPrinter *p) {
        ScheduleHtml::print(html, p);
    });
    preview.exec();
}

void MainWindow::onExportPdf() {
    if (!ensureEntitySelected()) return;

    const QString path = QFileDialog::getSaveFileName(this, "Експорт розкладу", m_entityCombo->currentText() + ".pdf", "PDF (*.pdf)");
    if (path.isEmpty()) return;

    if (ScheduleHtml::savePdf(currentHtml(), path)) {
        QMessageBox::information(this, "Експорт", "Розклад успішно збережено у PDF.");
    } else {
        QMessageBox::critical(this, "Помилка", "Не вдалося створити PDF-файл.");
    }
}

void MainWindow::onExportAllGroupsPdf() {
    const QString html = allGroupsHtml();
    if (html.isEmpty()) {
        QMessageBox::information(this, "Експорт", "Немає жодної групи для експорту.");
        return;
    }

    const QString path = QFileDialog::getSaveFileName(this, "Експорт розкладів усіх груп", "Розклади_всіх_груп.pdf", "PDF (*.pdf)");
    if (path.isEmpty()) return;

    if (ScheduleHtml::savePdf(html, path)) {
        QMessageBox::information(this, "Експорт", "Розклади всіх груп збережено в один PDF-файл.");
    } else {
        QMessageBox::critical(this, "Помилка", "Не вдалося створити PDF-файл.");
    }
}

void MainWindow::onPrint() {
    if (!ensureEntitySelected()) return;
    showPrintPreview(currentHtml());
}

void MainWindow::onPrintAllGroups() {
    const QString html = allGroupsHtml();
    if (html.isEmpty()) {
        QMessageBox::information(this, "Друк", "Немає жодної групи для друку.");
        return;
    }
    showPrintPreview(html);
}

bool MainWindow::saveScheduleXlsx(const QString &path) const {
    const ViewMode mode = currentMode();
    QXlsx::Document doc;
    const QString defaultSheet = doc.sheetNames().value(0);
    doc.addSheet("Розклад");

    QXlsx::Format titleFormat;
    titleFormat.setFontBold(true);
    titleFormat.setFontSize(14);
    titleFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);

    const QString title = QString("%1 %2 (%3)").arg(viewModeTitle(mode), m_entityCombo->currentText(), weekFilterTitle());
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
        doc.write(3, c + 2, scheduleDayNames().at(c), headerFormat);
    }

    for (int r = 0; r < m_grid->rowCount(); ++r) {
        const int excelRow = r + 4;
        doc.write(excelRow, 1, m_grid->pairHeaderAt(r), headerFormat);

        for (int c = 0; c < DaysCount; ++c) {
            QXlsx::Format cellFormat;
            cellFormat.setTextWrap(true);
            cellFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
            cellFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
            cellFormat.setBorderStyle(QXlsx::Format::BorderThin);

            QString text;
            if (const auto *item = m_grid->item(r, c)) {
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

    doc.addSheet("Дані");
    TableFile::writeRows(doc, ScheduleImporter::toRows(m_records));
    doc.deleteSheet(defaultSheet);
    doc.selectSheet("Розклад");

    return doc.saveAs(path);
}

void MainWindow::onExportTable() {
    if (!ensureEntitySelected()) return;

    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(this, "Експорт розкладу", m_entityCombo->currentText() + ".xlsx",
                                                "Excel (*.xlsx);;CSV (*.csv)", &selectedFilter);
    if (path.isEmpty()) return;
    if (!TableFile::hasKnownSuffix(path)) path += selectedFilter.contains("csv") ? ".csv" : ".xlsx";

    QString error;
    bool ok = false;
    if (TableFile::isXlsx(path)) {
        ok = saveScheduleXlsx(path);
        if (!ok) error = "не вдалося записати файл Excel.";
    } else {
        ok = Csv::write(path, ScheduleImporter::toRows(m_records), error);
    }

    if (ok) {
        QMessageBox::information(this, "Експорт", "Розклад успішно збережено. Файл містить дані для зворотного імпорту.");
    } else {
        QMessageBox::critical(this, "Помилка", "Не вдалося зберегти файл: " + error);
    }
}

void MainWindow::onExportAllData() {
    const auto records = DatabaseManager::instance().getAllScheduleRecords();
    if (records.isEmpty()) {
        QMessageBox::information(this, "Експорт", "Розклад порожній, експортувати нічого.");
        return;
    }

    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(this, "Експорт розкладу всіх груп", "Розклад_всіх_груп.xlsx",
                                                "Excel (*.xlsx);;CSV (*.csv)", &selectedFilter);
    if (path.isEmpty()) return;
    if (!TableFile::hasKnownSuffix(path)) path += selectedFilter.contains("csv") ? ".csv" : ".xlsx";

    QString error;
    const TableFile::Sheet sheet{"Дані", ScheduleImporter::toRows(records)};
    if (TableFile::writeSheets(path, {sheet}, error)) {
        QMessageBox::information(this, "Експорт", QString("Збережено пар: %1.").arg(records.size()));
    } else {
        QMessageBox::critical(this, "Помилка", "Не вдалося зберегти файл: " + error);
    }
}

void MainWindow::onImportSchedule() {
    ImportScheduleDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        loadEntities(currentEntityId());
    }
}

void MainWindow::onShowWorkload() {
    WorkloadDialog dlg(this);
    dlg.exec();
}

void MainWindow::onShowRoomLoad() {
    RoomLoadDialog dlg(this);
    dlg.exec();
}

void MainWindow::onShowConflicts() {
    ConflictReportDialog dlg(this);
    connect(&dlg, &ConflictReportDialog::groupRequested, this, [this](int groupId) {
        showGroup(groupId);
    });
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
    loadEntities(currentEntityId());

    if (ok) {
        QMessageBox::information(this, "Відновлення", "Базу даних успішно відновлено.");
    } else {
        QMessageBox::critical(this, "Помилка", error);
    }
}
