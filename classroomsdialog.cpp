#include "classroomsdialog.h"

ClassroomsDialog::ClassroomsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Керування аудиторіями");
    resize(460, 350);

    m_tableWidget = new QTableWidget(0, 3, this);
    m_tableWidget->setHorizontalHeaderLabels({"ID", "Номер аудиторії", "Місткість"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
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
    mainLayout->addWidget(m_tableWidget);
    mainLayout->addWidget(m_closeBtn);

    connect(m_addBtn, &QPushButton::clicked, this, &ClassroomsDialog::onAddClicked);
    connect(m_editBtn, &QPushButton::clicked, this, &ClassroomsDialog::onEditClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ClassroomsDialog::onDeleteClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    loadClassrooms();
}

void ClassroomsDialog::loadClassrooms() {
    m_tableWidget->setRowCount(0);
    const auto rooms = DatabaseManager::instance().getClassrooms();

    for (const auto &r : rooms) {
        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);

        auto *idItem = new QTableWidgetItem(QString::number(r.id));
        idItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 0, idItem);

        m_tableWidget->setItem(row, 1, new QTableWidgetItem(r.roomNumber));

        auto *capItem = new QTableWidgetItem(QString::number(r.capacity));
        capItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 2, capItem);
    }
}

void ClassroomsDialog::onAddClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle("Додати аудиторію");
    dlg.resize(320, 140);

    auto *roomEdit = new QLineEdit(&dlg);
    auto *capSpin = new QSpinBox(&dlg);
    capSpin->setRange(1, 500);
    capSpin->setValue(30);

    auto *form = new QFormLayout();
    form->addRow("Номер аудиторії:", roomEdit);
    form->addRow("Місткість (місць):", capSpin);

    auto *saveBtn = new QPushButton("Зберегти", &dlg);
    auto *cancelBtn = new QPushButton("Скасувати", &dlg);
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);

    auto *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);
    layout->addLayout(btnLayout);

    connect(saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString errorMsg;
        if (DatabaseManager::instance().addClassroom(roomEdit->text(), capSpin->value(), errorMsg)) {
            loadClassrooms();
        } else {
            QMessageBox::critical(this, "Помилка", errorMsg);
        }
    }
}

void ClassroomsDialog::onEditClicked() {
    int row = m_tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Інформація", "Оберіть аудиторію для редагування.");
        return;
    }

    int roomId = m_tableWidget->item(row, 0)->text().toInt();
    QString currentRoom = m_tableWidget->item(row, 1)->text();
    int currentCap = m_tableWidget->item(row, 2)->text().toInt();

    QDialog dlg(this);
    dlg.setWindowTitle("Редагувати аудиторію");
    dlg.resize(320, 140);

    auto *roomEdit = new QLineEdit(currentRoom, &dlg);
    auto *capSpin = new QSpinBox(&dlg);
    capSpin->setRange(1, 500);
    capSpin->setValue(currentCap > 0 ? currentCap : 30);

    auto *form = new QFormLayout();
    form->addRow("Номер аудиторії:", roomEdit);
    form->addRow("Місткість (місць):", capSpin);

    auto *saveBtn = new QPushButton("Зберегти", &dlg);
    auto *cancelBtn = new QPushButton("Скасувати", &dlg);
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);

    auto *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);
    layout->addLayout(btnLayout);

    connect(saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString errorMsg;
        if (DatabaseManager::instance().updateClassroom(roomId, roomEdit->text(), capSpin->value(), errorMsg)) {
            loadClassrooms();
        } else {
            QMessageBox::critical(this, "Помилка", errorMsg);
        }
    }
}

void ClassroomsDialog::onDeleteClicked() {
    int row = m_tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Інформація", "Оберіть аудиторію для видалення.");
        return;
    }

    int roomId = m_tableWidget->item(row, 0)->text().toInt();
    QString roomNumber = m_tableWidget->item(row, 1)->text();

    auto ans = QMessageBox::question(this, "Підтвердження",
                                    QString("Ви дійсно бажаєте видалити аудиторію '%1'?
Усі пов'язані заняття у розкладі також будуть видалені.").arg(roomNumber),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ans == QMessageBox::Yes) {
        QString errorMsg;
        if (DatabaseManager::instance().deleteClassroom(roomId, errorMsg)) {
            loadClassrooms();
        } else {
            QMessageBox::critical(this, "Помилка", errorMsg);
        }
    }
}
