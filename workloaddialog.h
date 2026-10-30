#ifndef WORKLOADDIALOG_H
#define WORKLOADDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QWidget>
#include "databasemanager.h"
#include "scheduleanalysis.h"

class BarChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit BarChartWidget(QWidget *parent = nullptr);
    void setData(const QString &title, const QStringList &labels, const QList<double> &values,
                 const QString &suffix = QString(), double fixedMax = 0);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QString m_suffix;
    QStringList m_labels;
    QList<double> m_values;
    double m_fixedMax = 0;
};

class WorkloadDialog : public QDialog {
    Q_OBJECT

public:
    explicit WorkloadDialog(QWidget *parent = nullptr);

private slots:
    void reload();
    void updateChart();
    void onShowScheduleClicked();

private:
    struct Row {
        int teacherId = 0;
        QString name;
        QString department;
        ScheduleAnalyzer::Load load;
    };

    QComboBox *m_weekCombo;
    QComboBox *m_metricCombo;
    QLabel *m_infoLabel;
    QTableWidget *m_tableWidget;
    BarChartWidget *m_chart;
    QList<Row> m_rows;
};

#endif // WORKLOADDIALOG_H
