#ifndef ROOMLOADDIALOG_H
#define ROOMLOADDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include "workloaddialog.h"

class RoomLoadDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoomLoadDialog(QWidget *parent = nullptr);

private slots:
    void reload();
    void onShowScheduleClicked();

private:
    struct Row {
        int roomId = 0;
        QString room;
        int capacity = 0;
        ScheduleAnalyzer::RoomLoad load;
    };

    QComboBox *m_weekCombo;
    QLabel *m_infoLabel;
    QTableWidget *m_tableWidget;
    BarChartWidget *m_chart;
    QList<Row> m_rows;
};

#endif // ROOMLOADDIALOG_H
