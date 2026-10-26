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
    void setData(const QString &title, const QStringList &labels, const QList<double> &values);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QStringList m_labels;
    QList<double> m_values;
};

class TeacherScheduleDialog : public QDialog {
    Q_OBJECT

public:
    TeacherScheduleDialog(int teacherId, const QString &teacherName, QWidget *parent = nullptr);

private slots:
    void reload();

private:
    int m_teacherId;
    QComboBox *m_weekCombo;
    QLabel *m_windowsLabel;
    QTableWidget *m_tableWidget;
    WindowsDelegate *m_delegate;
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