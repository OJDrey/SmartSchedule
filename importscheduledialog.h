#ifndef IMPORTSCHEDULEDIALOG_H
#define IMPORTSCHEDULEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTableWidget>
#include <QCheckBox>
#include <QPushButton>
#include "scheduleimporter.h"

class ImportScheduleDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportScheduleDialog(QWidget *parent = nullptr);

private slots:
    void onChooseFile();
    void refreshPreview();
    void onImportClicked();

private:
    QLabel *m_fileLabel;
    QLabel *m_summaryLabel;
    QTableWidget *m_tableWidget;
    QCheckBox *m_replaceCheck;
    QCheckBox *m_createCheck;
    QPushButton *m_chooseBtn;
    QPushButton *m_importBtn;
    QPushButton *m_cancelBtn;
    QList<ImportRow> m_rows;
};

#endif // IMPORTSCHEDULEDIALOG_H
