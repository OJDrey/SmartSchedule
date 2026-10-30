#ifndef BELLSDIALOG_H
#define BELLSDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimeEdit>
#include <QHeaderView>
#include <QMessageBox>
#include "databasemanager.h"

class BellsDialog : public QDialog {
    Q_OBJECT

public:
    explicit BellsDialog(QWidget *parent = nullptr);

private slots:
    void onAddRowClicked();
    void onDeleteRowClicked();
    void onSaveClicked();

private:
    void loadBells();
    void appendRow(const QTime &start, const QTime &end);
    void renumberRows();
    QTime timeAt(int row, int column) const;
    QString validate() const;

    QTableWidget *m_tableWidget;
    QPushButton *m_addBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // BELLSDIALOG_H
