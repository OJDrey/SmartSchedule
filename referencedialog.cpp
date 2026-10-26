#include "referencedialog.h"
#include "tablefile.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QColorDialog>
#include <QMessageBox>

ReferenceDialog::ReferenceDialog(const ReferenceConfig &config, QWidget *parent)
    : QDialog(parent), m_config(config)
{
    setWindowTitle(m_config.title);
    resize(520, 360);

    QStringList headers;
    for (const auto &field : m_config.fields) {
        headers << field.label;
    }

    m_tableWidget = new QTableWidget(0, headers.size(), this);
    m_tableWidget->setHorizontalHeaderLabels(headers);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_addBtn = new QPushButton("➕ Додати", this);
    m_editBtn = new QPushButton("✏️ Редагувати", this);
    m_deleteBtn = new QPushButton("❌ Видалити", this);
    m_closeBtn = new QPushButton("Закрити", this);

    auto *topBtnLayout = new QHBoxLayout();
    topBtnLayout->addWidget(m_addBtn);
    topBtnLayout->addWidget(m_editBtn);
    topBtnLayout->addWidget(m_deleteBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topBtnLayout);
    m_importBtn = new QPushButton("📥 Імпорт (Excel/CSV)", this);
    m_exportBtn = new QPushButton("📤 Експорт (Excel/CSV)", this);
    auto *csvLayout = new QHBoxLayout();
    csvLayout->addWidget(m_importBtn);
    csvLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(m_tableWidget);
    mainLayout->addLayout(csvLayout);
    mainLayout->addWidget(m_closeBtn);

    connect(m_addBtn, &QPushButton::clicked, this, &ReferenceDialog::onAddClicked);
    connect(m_editBtn, &QPushButton::clicked, this, &ReferenceDialog::onEditClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ReferenceDialog::onDeleteClicked);
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &ReferenceDialog::onEditClicked);
    connect(m_importBtn, &QPushButton::clicked, this, &ReferenceDialog::onImportClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &ReferenceDialog::onExportClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    reload();
}

void ReferenceDialog::reload() {
    m_tableWidget->setRowCount(0);
    const auto rows = m_config.load();
    m_tableWidget->setRowCount(rows.size());

    for (int r = 0; r < rows.size(); ++r) {
        for (int c = 0; c < m_config.fields.size(); ++c) {
            auto *item = new QTableWidgetItem(rows[r].values.value(c));
            const auto type = m_config.fields[c].type;

            if (type == ReferenceField::Number) {
                item->setTextAlignment(Qt::AlignCenter);
            } else if (type == ReferenceField::Color) {
                const QColor color(item->text());
                if (color.isValid()) {
                    item->setBackground(color);
                    item->setForeground(color.lightness() < 128 ? Qt::white : Qt::black);
                }
            }

            if (c == 0) item->setData(Qt::UserRole, rows[r].id);
            m_tableWidget->setItem(r, c, item);
        }
    }
}

int ReferenceDialog::currentId() const {
    const auto *item = m_tableWidget->item(m_tableWidget->currentRow(), 0);
    return item ? item->data(Qt::UserRole).toInt() : 0;
}

QStringList ReferenceDialog::currentValues() const {
    QStringList values;
    const int row = m_tableWidget->currentRow();
    for (int c = 0; c < m_tableWidget->columnCount(); ++c) {
        const auto *item = m_tableWidget->item(row, c);
        values << (item ? item->text() : QString());
    }
    return values;
}

bool ReferenceDialog::runEditor(int id, const QStringList &initial) {
    QDialog dlg(this);
    dlg.setWindowTitle(id > 0 ? "Редагування запису" : "Додавання запису");
    dlg.setMinimumWidth(340);

    auto *form = new QFormLayout();
    QList<std::function<QString()>> getters;

    for (int i = 0; i < m_config.fields.size(); ++i) {
        const auto &field = m_config.fields[i];
        const QString value = initial.value(i);

        if (field.type == ReferenceField::Number) {
            auto *spin = new QSpinBox(&dlg);
            spin->setRange(1, 500);
            spin->setValue(value.toInt() > 0 ? value.toInt() : 30);
            form->addRow(field.label + ":", spin);
            getters.append([spin] { return QString::number(spin->value()); });
        } else if (field.type == ReferenceField::Color) {
            auto *button = new QPushButton(&dlg);
            button->setProperty("color", QColor(value).isValid() ? value : QString("#4CAF50"));

            auto applyColor = [button] {
                const QString hex = button->property("color").toString();
                const QString textColor = QColor(hex).lightness() < 128 ? "white" : "black";
                button->setText(hex);
                button->setStyleSheet(QString("background: %1; color: %2;").arg(hex, textColor));
            };
            applyColor();

            connect(button, &QPushButton::clicked, &dlg, [button, applyColor, &dlg] {
                const QColor chosen = QColorDialog::getColor(QColor(button->property("color").toString()), &dlg);
                if (chosen.isValid()) {
                    button->setProperty("color", chosen.name());
                    applyColor();
                }
            });

            form->addRow(field.label + ":", button);
            getters.append([button] { return button->property("color").toString(); });
        } else {
            auto *edit = new QLineEdit(value, &dlg);
            form->addRow(field.label + ":", edit);
            getters.append([edit] { return edit->text(); });
        }
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    buttons->button(QDialogButtonBox::Ok)->setText("Зберегти");
    buttons->button(QDialogButtonBox::Cancel)->setText("Скасувати");

    auto *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&] {
        QStringList values;
        for (const auto &getter : getters) {
            values << getter();
        }

        QString error;
        if (m_config.save(id, values, error)) {
            dlg.accept();
        } else {
            QMessageBox::critical(&dlg, "Помилка", error);
        }
    });

    return dlg.exec() == QDialog::Accepted;
}

void ReferenceDialog::onAddClicked() {
    if (runEditor(0, QStringList())) {
        reload();
    }
}

void ReferenceDialog::onEditClicked() {
    const int id = currentId();
    if (id <= 0) {
        QMessageBox::information(this, "Інформація", "Оберіть запис для редагування.");
        return;
    }

    if (runEditor(id, currentValues())) {
        reload();
    }
}

void ReferenceDialog::onDeleteClicked() {
    const int id = currentId();
    if (id <= 0) {
        QMessageBox::information(this, "Інформація", "Оберіть запис для видалення.");
        return;
    }

    const QString name = m_tableWidget->item(m_tableWidget->currentRow(), 0)->text();
    const int lessons = m_config.usage ? m_config.usage(id) : 0;

    QString question = QString("Видалити запис «%1»?").arg(name);
    if (lessons > 0) {
        question += kNewLine + QString("Разом з ним буде видалено занять у розкладі: %1.").arg(lessons);
    }

    if (QMessageBox::question(this, "Підтвердження", question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (m_config.remove(id, error)) {
        reload();
    } else {
        QMessageBox::critical(this, "Помилка", error);
    }
}

void ReferenceDialog::onImportClicked() {
    const QString path = QFileDialog::getOpenFileName(this, "Імпорт довідника", QString(),
                                                      "Таблиці (*.xlsx *.csv *.txt);;Excel (*.xlsx);;CSV (*.csv *.txt)");
    if (path.isEmpty()) return;

    QString error;
    auto rows = TableFile::read(path, error);
    if (rows.isEmpty()) {
        QMessageBox::warning(this, "Імпорт", error.isEmpty() ? QString("Файл порожній.") : error);
        return;
    }

    if (rows.first().value(0).trimmed().compare(m_config.fields.first().label, Qt::CaseInsensitive) == 0) {
        rows.removeFirst();
    }

    const int columns = m_config.fields.size();
    int imported = 0;
    QStringList problems;

    for (const auto &row : rows) {
        QStringList values = row.mid(0, columns);
        while (values.size() < columns) {
            values << QString();
        }

        QString rowError;
        if (m_config.save(0, values, rowError)) {
            ++imported;
        } else if (problems.size() < 5) {
            problems << QString("«%1»: %2").arg(values.first(), rowError);
        }
    }

    QString report = QString("Імпортовано записів: %1").arg(imported);
    const int skipped = rows.size() - imported;
    if (skipped > 0) {
        report += kNewLine + QString("Пропущено: %1").arg(skipped) + kNewLine + problems.join(kNewLine);
    }

    QMessageBox::information(this, "Імпорт", report);
    reload();
}

void ReferenceDialog::onExportClicked() {
    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(this, "Експорт довідника", m_config.title + ".xlsx",
                                                "Excel (*.xlsx);;CSV (*.csv)", &selectedFilter);
    if (path.isEmpty()) return;
    if (!TableFile::hasKnownSuffix(path)) path += selectedFilter.contains("csv") ? ".csv" : ".xlsx";

    QStringList header;
    for (const auto &field : m_config.fields) {
        header << field.label;
    }

    QList<QStringList> rows;
    rows.append(header);
    for (const auto &row : m_config.load()) {
        rows.append(row.values);
    }

    QString error;
    if (TableFile::write(path, rows, error)) {
        QMessageBox::information(this, "Експорт", "Дані успішно збережено.");
    } else {
        QMessageBox::critical(this, "Помилка", "Не вдалося зберегти файл: " + error);
    }
}