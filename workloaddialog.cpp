#include "workloaddialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollArea>
#include <QPainter>
#include <algorithm>

namespace {
constexpr int Margin = 12;
constexpr int LabelWidth = 190;
constexpr int RowHeight = 28;
constexpr int TitleHeight = 30;
constexpr int ValueWidth = 56;

QString pairHeader(const BellItem &b) {
    return QString("%1 пара").arg(b.pairNumber) + kNewLine + QString("%1 - %2").arg(b.startTime, b.endTime);
}
}

BarChartWidget::BarChartWidget(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);
}

void BarChartWidget::setData(const QString &title, const QStringList &labels, const QList<double> &values) {
    m_title = title;
    m_labels = labels;
    m_values = values;
    setMinimumHeight(TitleHeight + 2 * Margin + static_cast<int>(m_values.size()) * RowHeight);
    update();
}

void BarChartWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(palette().text().color());

    QFont titleFont = font();
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(QRect(Margin, Margin / 2, width() - 2 * Margin, TitleHeight), Qt::AlignVCenter | Qt::AlignLeft, m_title);
    p.setFont(font());

    if (m_values.isEmpty()) {
        p.drawText(rect(), Qt::AlignCenter, "Немає даних");
        return;
    }

    double maxValue = *std::max_element(m_values.cbegin(), m_values.cend());
    if (maxValue <= 0) maxValue = 1;

    const int barsLeft = Margin + LabelWidth + Margin;
    const int barAreaWidth = qMax(20, width() - barsLeft - ValueWidth - 2 * Margin);
    const int top = Margin + TitleHeight;

    p.drawLine(barsLeft, top, barsLeft, top + static_cast<int>(m_values.size()) * RowHeight);

    for (int i = 0; i < m_values.size(); ++i) {
        const int y = top + i * RowHeight;

        p.setPen(palette().text().color());
        p.drawText(QRect(Margin, y, LabelWidth, RowHeight), Qt::AlignVCenter | Qt::AlignRight,
                   fontMetrics().elidedText(m_labels.value(i), Qt::ElideRight, LabelWidth - 8));

        const int barWidth = static_cast<int>(barAreaWidth * m_values[i] / maxValue);
        const QRect bar(barsLeft + 1, y + 5, qMax(barWidth, 1), RowHeight - 10);
        p.fillRect(bar, palette().highlight());

        p.drawText(QRect(bar.right() + 6, y, ValueWidth, RowHeight), Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(m_values[i], 'f', 1));
    }
}

TeacherScheduleDialog::TeacherScheduleDialog(int teacherId, const QString &teacherName, QWidget *parent)
    : QDialog(parent), m_teacherId(teacherId)
{
    setWindowTitle("Розклад викладача: " + teacherName);
    resize(980, 620);

    m_weekCombo = new QComboBox(this);
    m_weekCombo->addItem("📅 Усі тижні", 0);
    for (int w = 1; w <= 4; ++w) {
        m_weekCombo->addItem(weekTypeName(w), w);
    }

    m_windowsLabel = new QLabel(this);
    m_windowsLabel->setStyleSheet("font-weight: bold;");

    m_tableWidget = new QTableWidget(0, 5, this);
    m_tableWidget->setHorizontalHeaderLabels({"Понеділок", "Вівторок", "Середа", "Четвер", "П'ятниця"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    m_delegate = new WindowsDelegate(m_tableWidget);
    m_tableWidget->setItemDelegate(m_delegate);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Фільтр тижня:", this));
    topLayout->addWidget(m_weekCombo);
    topLayout->addWidget(m_windowsLabel, 1);

    auto *closeBtn = new QPushButton("Закрити", this);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_tableWidget);
    mainLayout->addWidget(closeBtn);

    connect(m_weekCombo, &QComboBox::currentIndexChanged, this, &TeacherScheduleDialog::reload);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    reload();
}

void TeacherScheduleDialog::reload() {
    m_tableWidget->clearContents();

    auto &db = DatabaseManager::instance();
    const auto bells = db.getBellSchedule();
    m_tableWidget->setRowCount(bells.size());

    QStringList headers;
    for (const auto &b : bells) {
        headers << pairHeader(b);
    }
    m_tableWidget->setVerticalHeaderLabels(headers);

    const int week = m_weekCombo->currentData().toInt();
    const auto records = db.getScheduleForTeacher(m_teacherId, week);

    QMap<QPair<int, int>, QList<ScheduleRecord>> cells;
    for (const auto &rec : records) {
        cells[qMakePair(rec.pairNumber - 1, rec.dayOfWeek - 1)].append(rec);
    }

    const QString separator = QString(kNewLine) + "-----------------" + kNewLine;

    for (auto it = cells.constBegin(); it != cells.constEnd(); ++it) {
        const int row = it.key().first;
        const int col = it.key().second;
        const auto &list = it.value();
        if (row < 0 || row >= bells.size() || col < 0 || col >= 5) continue;

        QStringList lines;
        for (const auto &rec : list) {
            const QString prefix = rec.weekType > 0 ? QString("[%1 тижд] ").arg(rec.weekType) : QString();
            lines << prefix + rec.subjectName + kNewLine + QString("Група: %1 | Ауд: %2").arg(rec.groupName, rec.roomNumber);
        }

        auto *item = new QTableWidgetItem(lines.join(separator));
        item->setTextAlignment(Qt::AlignCenter);

        QColor bgColor = list.size() > 1 ? QColor("#E3F2FD") : QColor(list.first().colorHex);
        if (!bgColor.isValid()) bgColor = QColor("#4CAF50");
        item->setBackground(bgColor);
        item->setForeground(bgColor.lightness() < 128 ? Qt::white : Qt::black);

        m_tableWidget->setItem(row, col, item);
    }

    m_delegate->windows.clear();
    const auto windows = ScheduleAnalyzer::findWindows(records, week);
    for (const auto &w : windows) {
        m_delegate->windows.insert(WindowsDelegate::key(w.pair - 1, w.day - 1));
    }
    m_windowsLabel->setText(QString("Вікон у розкладі: %1").arg(windows.size()));
    m_tableWidget->viewport()->update();
}

WorkloadDialog::WorkloadDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Навантаження викладачів");
    resize(820, 640);

    m_weekCombo = new QComboBox(this);
    m_weekCombo->addItem("Середнє за цикл тижнів", 0);
    for (int w = 1; w <= 4; ++w) {
        m_weekCombo->addItem(weekTypeName(w), w);
    }

    m_metricCombo = new QComboBox(this);
    m_metricCombo->addItem("Годин на тиждень", 0);
    m_metricCombo->addItem("Пар на тиждень", 1);
    m_metricCombo->addItem("Вікон на тиждень", 2);

    m_infoLabel = new QLabel(this);

    m_tableWidget = new QTableWidget(0, 5, this);
    m_tableWidget->setHorizontalHeaderLabels({"Викладач", "Кафедра", "Пар / тиждень", "Годин / тиждень", "Вікон / тиждень"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_chart = new BarChartWidget(this);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_chart);

    auto *scheduleBtn = new QPushButton("📅 Розклад викладача та вікна...", this);
    auto *closeBtn = new QPushButton("Закрити", this);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Тиждень:", this));
    topLayout->addWidget(m_weekCombo);
    topLayout->addWidget(new QLabel("Діаграма:", this));
    topLayout->addWidget(m_metricCombo);
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

    connect(m_weekCombo, &QComboBox::currentIndexChanged, this, &WorkloadDialog::reload);
    connect(m_metricCombo, &QComboBox::currentIndexChanged, this, &WorkloadDialog::updateChart);
    connect(scheduleBtn, &QPushButton::clicked, this, &WorkloadDialog::onShowScheduleClicked);
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &WorkloadDialog::onShowScheduleClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    reload();
}

void WorkloadDialog::reload() {
    auto &db = DatabaseManager::instance();
    const auto teachers = db.getTeachers();
    const auto hours = ScheduleAnalyzer::pairHours(db.getBellSchedule());

    QMap<int, QList<ScheduleRecord>> records;
    int cycle = 1;
    for (const auto &t : teachers) {
        records[t.id] = db.getScheduleForTeacher(t.id, 0);
        cycle = qMax(cycle, ScheduleAnalyzer::cycleLength(records[t.id]));
    }

    const int week = m_weekCombo->currentData().toInt();
    m_rows.clear();
    for (const auto &t : teachers) {
        Row row;
        row.teacherId = t.id;
        row.name = t.name;
        row.department = t.department;
        row.load = ScheduleAnalyzer::computeLoad(records[t.id], hours, week, cycle);
        m_rows.append(row);
    }

    auto numberItem = [](double value) {
        auto *item = new QTableWidgetItem(QString::number(value, 'f', 1));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(m_rows.size());
    for (int i = 0; i < m_rows.size(); ++i) {
        const Row &r = m_rows[i];
        auto *nameItem = new QTableWidgetItem(r.name);
        nameItem->setData(Qt::UserRole, r.teacherId);
        m_tableWidget->setItem(i, 0, nameItem);
        m_tableWidget->setItem(i, 1, new QTableWidgetItem(r.department));
        m_tableWidget->setItem(i, 2, numberItem(r.load.lessons));
        m_tableWidget->setItem(i, 3, numberItem(r.load.hours));
        m_tableWidget->setItem(i, 4, numberItem(r.load.windows));
    }

    m_infoLabel->setText(QString("Довжина циклу тижнів у розкладі: %1. Година = реальна тривалість пар за розкладом дзвінків.").arg(cycle));
    updateChart();
}

void WorkloadDialog::updateChart() {
    const int metric = m_metricCombo->currentData().toInt();
    auto valueOf = [metric](const Row &r) {
        return metric == 0 ? r.load.hours : (metric == 1 ? r.load.lessons : r.load.windows);
    };

    QList<Row> sorted = m_rows;
    std::sort(sorted.begin(), sorted.end(), [&](const Row &a, const Row &b) {
        return valueOf(a) > valueOf(b);
    });

    QStringList names;
    QList<double> values;
    for (const auto &r : sorted) {
        names << r.name;
        values << valueOf(r);
    }
    m_chart->setData(m_metricCombo->currentText(), names, values);
}

void WorkloadDialog::onShowScheduleClicked() {
    const auto *item = m_tableWidget->item(m_tableWidget->currentRow(), 0);
    if (!item) {
        QMessageBox::information(this, "Інформація", "Оберіть викладача у таблиці.");
        return;
    }

    TeacherScheduleDialog dlg(item->data(Qt::UserRole).toInt(), item->text(), this);
    dlg.exec();
}