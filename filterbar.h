#ifndef FILTERBAR_H
#define FILTERBAR_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include "schedulefilter.h"

class FilterBar : public QWidget {
    Q_OBJECT

public:
    explicit FilterBar(QWidget *parent = nullptr);

    ScheduleFilter filter() const;
    void reloadLists();

signals:
    void filterChanged();

private:
    void fillCombo(QComboBox *combo, const QString &allText, const QList<QPair<int, QString>> &items);

    QLineEdit *m_searchEdit;
    QComboBox *m_teacherCombo;
    QComboBox *m_subjectCombo;
    QComboBox *m_roomCombo;
    QPushButton *m_clearBtn;
};

#endif // FILTERBAR_H
