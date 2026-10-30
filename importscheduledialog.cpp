#include "importscheduledialog.h"
#include "tablefile.h"
#include "schedulefilter.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

ImportScheduleDialog::ImportScheduleDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Імпорт розкладу з Excel / CSV");
    resize(980, 560);

    m_fileLabel = new QLabel("Файл не вибрано. Використовуйте файл, експортований цією програмою (аркуш «Дані»).", this);
    m_fileLabel->setWordWrap(true);
    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setStyleSheet("font-weight: bold;");

    m_chooseBtn = new QPushButton("📂 Вибрати файл...", this);
    m_importBtn = new QPushButton("📥 Імпортувати", this);
    m_importBtn->setEnabled(false);
    m_cancelBtn = new QPushButton("Скасувати", this);

    m_replaceCheck = new QCheckBox("Замінити розклад груп, що є у файлі (старі пари цих груп буде видалено)", this);
    m_createCheck = new QCheckBox("Створювати відсутні групи, предмети, викладачів та аудиторії", this);
    m_createCheck->setChecked(true);

    const QStringList headers = {"Рядок", "Група", "День", "Пара", "Тиждень", "Предмет", "Викладач", "Аудиторія", "Статус"};
    m_tableWidget = new QTableWidget(0, headers.size(), this);
    m_tableWidget->setHorizontalHeaderLabels(headers);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(m_chooseBtn);
    topLayout->addWidget(m_fileLabel, 1);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(m_summaryLabel, 1);
    bottomLayout->addWidget(m_importBtn);
    bottomLayout->addWidget(m_cancelBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_replaceCheck);
    mainLayout->addWidget(m_createCheck);
    mainLayout->addWidget(m_tableWidget);
    mainLayout->addLayout(bottomLayout);

    connect(m_chooseBtn, &QPushButton::clicked, this, &ImportScheduleDialog::onChooseFile);
    connect(m_importBtn, &QPushButton::clicked, this, &ImportScheduleDialog::onImportClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_createCheck, &QCheckBox::toggled, this, &ImportScheduleDialog::refreshPreview);
}

void ImportScheduleDialog::onChooseFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Імпорт розкладу", QString(),
                                                      "Таблиці (*.xlsx *.csv);;Excel (*.xlsx);;CSV (*.csv)");
    if (path.isEmpty()) return;

    QString error;
    const auto rows = TableFile::readSheet(path, "Дані", error);
    if (rows.isEmpty()) {
        QMessageBox::warning(this, "Імпорт", error.isEmpty() ? QString("Файл порожній.") : error);
        return;
    }

    m_fileLabel->setText(path);
    m_rows = ScheduleImporter::parse(rows);
    refreshPreview();
}

void ImportScheduleDialog::refreshPreview() {
    ScheduleImporter::resolveReferences(m_rows);

    const bool create = m_createCheck->isChecked();
    int ready = 0;
    int broken = 0;

    m_tableWidget->setRowCount(m_rows.size());
    for (int i = 0; i < m_rows.size(); ++i) {
        const ImportRow &row = m_rows[i];

        QString status;
        bool ok = true;
        if (!row.error.isEmpty()) {
            status = "❌ " + row.error;
            ok = false;
        } else if (!row.missing.isEmpty()) {
            if (create) {
                status = "➕ Буде створено: " + row.missing.join(", ");
            } else {
                status = "❌ Немає в довідниках: " + row.missing.join(", ");
                ok = false;
            }
        } else {
            status = "✅ Готово";
        }

        ok ? ++ready : ++broken;

        const QStringList cells = {
            QString::number(row.sourceRow), row.groupName, scheduleDayNames().value(row.record.dayOfWeek - 1),
            QString::number(row.record.pairNumber), weekTypeName(row.record.weekType),
            row.subjectName, row.teacherName, row.roomName, status
        };
        for (int c = 0; c < cells.size(); ++c) {
            auto *item = new QTableWidgetItem(cells[c]);
            if (!ok) item->setForeground(QColor("#E53935"));
            m_tableWidget->setItem(i, c, item);
        }
    }

    m_summaryLabel->setText(QString("Рядків: %1 | готово до імпорту: %2 | з помилками: %3").arg(m_rows.size()).arg(ready).arg(broken));
    m_importBtn->setEnabled(ready > 0);
}

void ImportScheduleDialog::onImportClicked() {
    if (m_replaceCheck->isChecked()) {
        const auto answer = QMessageBox::warning(this, "Підтвердження",
                                                 "Поточний розклад груп, що є у файлі, буде видалено і замінено даними з файлу. Продовжити?",
                                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) return;
    }

    const ImportReport report = ScheduleImporter::run(m_rows, m_replaceCheck->isChecked(), m_createCheck->isChecked());

    QString text = QString("Імпортовано пар: %1 з %2.").arg(report.imported).arg(report.total);
    if (!report.errors.isEmpty()) {
        text += QString(kNewLine) + kNewLine + "Пропущено:" + kNewLine + report.errors.mid(0, 10).join(kNewLine);
        if (report.errors.size() > 10) text += QString(kNewLine) + QString("... та ще %1").arg(report.errors.size() - 10);
    }

    QMessageBox::information(this, "Результат імпорту", text);
    if (report.imported > 0) accept();
}
