#ifndef CONFLICTREPORTDIALOG_H
#define CONFLICTREPORTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTableWidget>
#include "scheduleanalysis.h"

class ConflictReportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConflictReportDialog(QWidget *parent = nullptr);

signals:
    void groupRequested(int groupId);

private slots:
    void runCheck();
    void onRowActivated(int row);

private:
    QLabel *m_summaryLabel;
    QTableWidget *m_tableWidget;
    QList<ScheduleAnalyzer::Conflict> m_conflicts;
};

#endif // CONFLICTREPORTDIALOG_H
