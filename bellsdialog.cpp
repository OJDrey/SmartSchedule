#include "bellsdialog.h"

BellsDialog::BellsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Налаштування розкладу дзвінків");
    resize(420, 320);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_tableWidget = new QTableWidget(0, 3, this);
    m_tableWidget->setHorizontalHeaderLabels({"№ пари", "Час початку", "Час завершення"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QHBoxLayout *topBtnLayout = new QHBoxLayout();
    m_addBtn = new QPushButton("➕ Додати пару", this);
    m_deleteBtn = new QPushButton("❌ Видалити пару", this);
    topBtnLayout->addWidget(m_addBtn);
    topBtnLayout->addWidget(m_deleteBtn);

    QHBoxLayout *bottomBtnLayout = new QHBoxLayout();
    m_saveBtn = new QPushButton("Зберегти зміни", this);
    m_cancelBtn = new QPushButton("Скасувати", this);
    bottomBtnLayout->addWidget(m_saveBtn);
    bottomBtnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(topBtnLayout);
    mainLayout->addWidget(m_tableWidget);
    mainLayout->addLayout(bottomBtnLayout);

    connect(m_addBtn, &QPushButton::clicked, this, &BellsDialog::onAddRowClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &BellsDialog::onDeleteRowClicked);
    connect(m_saveBtn, &QPushButton::clicked, this, &BellsDialog::onSaveClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    loadBells();
}

void BellsDialog::loadBells() {
    m_tableWidget->setRowCount(0);
    auto bells = DatabaseManager::instance().getBellSchedule();

    for (const auto &b : bells) {
        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);

        QTableWidgetItem *numItem = new QTableWidgetItem(QString::number(b.pairNumber));
        numItem->setFlags(numItem->flags() & ~Qt::ItemIsEditable);
        numItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 0, numItem);

        QTimeEdit *startTime = new QTimeEdit(QTime::fromString(b.startTime, "hh:mm"), this);
        startTime->setDisplayFormat("hh:mm");
        m_tableWidget->setCellWidget(row, 1, startTime);

        QTimeEdit *endTime = new QTimeEdit(QTime::fromString(b.endTime, "hh:mm"), this);
        endTime->setDisplayFormat("hh:mm");
        m_tableWidget->setCellWidget(row, 2, endTime);
    }
}

void BellsDialog::onAddRowClicked() {
    int row = m_tableWidget->rowCount();
    int pairNum = row + 1;

    m_tableWidget->insertRow(row);

    QTableWidgetItem *numItem = new QTableWidgetItem(QString::number(pairNum));
    numItem->setFlags(numItem->flags() & ~Qt::ItemIsEditable);
    numItem->setTextAlignment(Qt::AlignCenter);
    m_tableWidget->setItem(row, 0, numItem);

    QTimeEdit *startTime = new QTimeEdit(QTime(8 + pairNum, 0), this);
    startTime->setDisplayFormat("hh:mm");
    m_tableWidget->setCellWidget(row, 1, startTime);

    QTimeEdit *endTime = new QTimeEdit(QTime(8 + pairNum, 45), this);
    endTime->setDisplayFormat("hh:mm");
    m_tableWidget->setCellWidget(row, 2, endTime);
}

void BellsDialog::onDeleteRowClicked() {
    int row = m_tableWidget->rowCount();
    if (row > 0) {
        m_tableWidget->removeRow(row - 1);
    }
}

void BellsDialog::onSaveClicked() {
    QList<BellItem> bells;
    for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
        BellItem b;
        b.pairNumber = i + 1;

        QTimeEdit *startEdit = qobject_cast<QTimeEdit*>(m_tableWidget->cellWidget(i, 1));
        QTimeEdit *endEdit = qobject_cast<QTimeEdit*>(m_tableWidget->cellWidget(i, 2));

        if (startEdit && endEdit) {
            b.startTime = startEdit->time().toString("hh:mm");
            b.endTime = endEdit->time().toString("hh:mm");
            bells.append(b);
        }
    }

    if (DatabaseManager::instance().updateBellSchedule(bells)) {
        QMessageBox::information(this, "Успіх", "Розклад дзвінків успішно оновлено!");
        accept();
    } else {
        QMessageBox::critical(this, "Помилка", "Не вдалося зберегти розклад дзвінків у базу даних.");
    }
}