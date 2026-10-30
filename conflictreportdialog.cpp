#include "conflictreportdialog.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>

ConflictReportDialog::ConflictReportDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Звіт про конфлікти розкладу");
    resize(860, 460);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setStyleSheet("font-weight: bold;");

    m_tableWidget = new QTableWidget(0, 4, this);
    m_tableWidget->setHorizontalHeaderLabels({"Тип", "День", "Пара", "Опис"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    auto *recheckBtn = new QPushButton("🔄 Перевірити знову", this);
    auto *closeBtn = new QPushButton("Закрити", this);
    auto *hint = new QLabel("Подвійний клік по рядку відкриває розклад відповідної групи.", this);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(hint, 1);
    bottomLayout->addWidget(recheckBtn);
    bottomLayout->addWidget(closeBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_summaryLabel);
    mainLayout->addWidget(m_tableWidget);
    mainLayout->addLayout(bottomLayout);

    connect(recheckBtn, &QPushButton::clicked, this, &ConflictReportDialog::runCheck);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &ConflictReportDialog::onRowActivated);

    runCheck();
}

void ConflictReportDialog::runCheck() {
    auto &db = DatabaseManager::instance();
    m_conflicts = ScheduleAnalyzer::findConflicts(db.getAllScheduleRecords(), db.getBellSchedule());

    m_tableWidget->setRowCount(m_conflicts.size());
    for (int i = 0; i < m_conflicts.size(); ++i) {
        const auto &c = m_conflicts[i];
        const QStringList cells = {c.type, scheduleDayNames().value(c.day - 1), QString::number(c.pair), c.description};
        for (int col = 0; col < cells.size(); ++col) {
            m_tableWidget->setItem(i, col, new QTableWidgetItem(cells[col]));
        }
    }

    if (m_conflicts.isEmpty()) {
        m_summaryLabel->setText("✅ Конфліктів не знайдено. Усю базу розкладу перевірено.");
    } else {
        m_summaryLabel->setText(QString("⚠️ Знайдено конфліктів: %1").arg(m_conflicts.size()));
    }
}

void ConflictReportDialog::onRowActivated(int row) {
    if (row < 0 || row >= m_conflicts.size()) return;
    emit groupRequested(m_conflicts[row].groupId);
    accept();
}
