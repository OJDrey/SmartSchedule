#include "teachersdialog.h"

TeachersDialog::TeachersDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Керування викладачами");
    resize(520, 360);

    m_tableWidget = new QTableWidget(0, 3, this);
    m_tableWidget->setHorizontalHeaderLabels({"ID", "ПІБ викладача", "Кафедра"});
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

    connect(m_addBtn, &QPushButton::clicked, this, &TeachersDialog::onAddClicked);
    connect(m_editBtn, &QPushButton::clicked, this, &TeachersDialog::onEditClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TeachersDialog::onDeleteClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    loadTeachers();
}

void TeachersDialog::loadTeachers() {
    m_tableWidget->setRowCount(0);
    const auto teachers = DatabaseManager::instance().getTeachers();

    for (const auto &t : teachers) {
        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);

        auto *idItem = new QTableWidgetItem(QString::number(t.id));
        idItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 0, idItem);

        m_tableWidget->setItem(row, 1, new QTableWidgetItem(t.name));
        m_tableWidget->setItem(row, 2, new QTableWidgetItem(t.department));
    }
}

void TeachersDialog::onAddClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle("Додати викладача");
    dlg.resize(360, 150);

    auto *nameEdit = new QLineEdit(&dlg);
    auto *deptEdit = new QLineEdit(&dlg);

    auto *form = new QFormLayout();
    form->addRow("ПІБ викладача:", nameEdit);
    form->addRow("Кафедра:", deptEdit);

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
        if (DatabaseManager::instance().addTeacher(nameEdit->text(), deptEdit->text(), errorMsg)) {
            loadTeachers();
        } else {
            QMessageBox::critical(this, "Помилка", errorMsg);
        }
    }
}

void TeachersDialog::onEditClicked() {
    int row = m_tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Інформація", "Оберіть викладача для редагування.");
        return;
    }

    int teacherId = m_tableWidget->item(row, 0)->text().toInt();
    QString currentName = m_tableWidget->item(row, 1)->text();
    QString currentDept = m_tableWidget->item(row, 2)->text();

    QDialog dlg(this);
    dlg.setWindowTitle("Редагувати викладача");
    dlg.resize(360, 150);

    auto *nameEdit = new QLineEdit(currentName, &dlg);
    auto *deptEdit = new QLineEdit(currentDept, &dlg);

    auto *form = new QFormLayout();
    form->addRow("ПІБ викладача:", nameEdit);
    form->addRow("Кафедра:", deptEdit);

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
        if (DatabaseManager::instance().updateTeacher(teacherId, nameEdit->text(), deptEdit->text(), errorMsg)) {
            loadTeachers();
        } else {
            QMessageBox::critical(this, "Помилка", errorMsg);
        }
    }
}

void TeachersDialog::onDeleteClicked() {
    int row = m_tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Інформація", "Оберіть викладача для видалення.");
        return;
    }

    int teacherId = m_tableWidget->item(row, 0)->text().toInt();
    QString teacherName = m_tableWidget->item(row, 1)->text();

    auto ans = QMessageBox::question(this, "Підтвердження",
                                    QString("Ви дійсно бажаєте видалити викладача '%1'?
Усі пов'язані заняття у розкладі також будуть видалені.").arg(teacherName),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ans == QMessageBox::Yes) {
        QString errorMsg;
        if (DatabaseManager::instance().deleteTeacher(teacherId, errorMsg)) {
            loadTeachers();
        } else {
            QMessageBox::critical(this, "Помилка", errorMsg);
        }
    }
}
