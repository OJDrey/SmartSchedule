#include "bellsdialog.h"

BellsDialog::BellsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Налаштування розкладу дзвінків");
    resize(420, 320);

    m_tableWidget = new QTableWidget(0, 3, this);
    m_tableWidget->setHorizontalHeaderLabels({"№ пари", "Час початку", "Час завершення"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    m_addBtn = new QPushButton("➕ Додати пару", this);
    m_deleteBtn = new QPushButton("❌ Видалити пару", this);
    m_saveBtn = new QPushButton("Зберегти зміни", this);
    m_cancelBtn = new QPushButton("Скасувати", this);

    auto *topBtnLayout = new QHBoxLayout();
    topBtnLayout->addWidget(m_addBtn);
    topBtnLayout->addWidget(m_deleteBtn);

    auto *bottomBtnLayout = new QHBoxLayout();
    bottomBtnLayout->addWidget(m_saveBtn);
    bottomBtnLayout->addWidget(m_cancelBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topBtnLayout);
    mainLayout->addWidget(m_tableWidget);
    mainLayout->addLayout(bottomBtnLayout);

    connect(m_addBtn, &QPushButton::clicked, this, &BellsDialog::onAddRowClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &BellsDialog::onDeleteRowClicked);
    connect(m_saveBtn, &QPushButton::clicked, this, &BellsDialog::onSaveClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    loadBells();
}

void BellsDialog::appendRow(const QTime &start, const QTime &end) {
    const int row = m_tableWidget->rowCount();
    m_tableWidget->insertRow(row);

    auto *numItem = new QTableWidgetItem(QString::number(row + 1));
    numItem->setFlags(numItem->flags() & ~Qt::ItemIsEditable);
    numItem->setTextAlignment(Qt::AlignCenter);
    m_tableWidget->setItem(row, 0, numItem);

    for (int column = 1; column <= 2; ++column) {
        auto *edit = new QTimeEdit(column == 1 ? start : end, this);
        edit->setDisplayFormat("hh:mm");
        m_tableWidget->setCellWidget(row, column, edit);
    }
}

void BellsDialog::renumberRows() {
    for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
        m_tableWidget->item(i, 0)->setText(QString::number(i + 1));
    }
}

QTime BellsDialog::timeAt(int row, int column) const {
    auto *edit = qobject_cast<QTimeEdit*>(m_tableWidget->cellWidget(row, column));
    return edit ? edit->time() : QTime();
}

void BellsDialog::loadBells() {
    m_tableWidget->setRowCount(0);
    for (const auto &b : DatabaseManager::instance().getBellSchedule()) {
        appendRow(QTime::fromString(b.startTime, "hh:mm"), QTime::fromString(b.endTime, "hh:mm"));
    }
}

void BellsDialog::onAddRowClicked() {
    const int count = m_tableWidget->rowCount();
    QTime start(8, 30);
    if (count > 0) {
        start = timeAt(count - 1, 2).addSecs(20 * 60);
    }
    appendRow(start, start.addSecs(80 * 60));
}

void BellsDialog::onDeleteRowClicked() {
    int row = m_tableWidget->currentRow();
    if (row < 0) row = m_tableWidget->rowCount() - 1;
    if (row < 0) return;

    m_tableWidget->removeRow(row);
    renumberRows();
}

QString BellsDialog::validate() const {
    QTime previousEnd;
    for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
        const QTime start = timeAt(i, 1);
        const QTime end = timeAt(i, 2);

        if (!start.isValid() || !end.isValid())
            return QString("Некоректний час у %1-й парі.").arg(i + 1);
        if (start >= end)
            return QString("У %1-й парі час початку має бути раніше за час завершення.").arg(i + 1);
        if (previousEnd.isValid() && start < previousEnd)
            return QString("%1-а пара починається раніше, ніж завершується попередня.").arg(i + 1);

        previousEnd = end;
    }
    return QString();
}

void BellsDialog::onSaveClicked() {
    const QString validationError = validate();
    if (!validationError.isEmpty()) {
        QMessageBox::warning(this, "Некоректні дані", validationError);
        return;
    }

    QList<BellItem> bells;
    for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
        bells.append({i + 1, timeAt(i, 1).toString("hh:mm"), timeAt(i, 2).toString("hh:mm")});
    }

    QString errorMsg;
    if (!DatabaseManager::instance().updateBellSchedule(bells, errorMsg)) {
        QMessageBox::critical(this, "Помилка", QString("Не вдалося зберегти розклад дзвінків:") + kNewLine + errorMsg);
        return;
    }

    QMessageBox::information(this, "Успіх", "Розклад дзвінків успішно оновлено!");
    accept();
}