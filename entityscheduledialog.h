#ifndef ENTITYSCHEDULEDIALOG_H
#define ENTITYSCHEDULEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include "schedulegrid.h"

class EntityScheduleDialog : public QDialog {
    Q_OBJECT

public:
    EntityScheduleDialog(ViewMode mode, int entityId, const QString &name, QWidget *parent = nullptr);

private slots:
    void reload();

private:
    ViewMode m_mode;
    int m_entityId;
    QComboBox *m_weekCombo;
    QLabel *m_windowsLabel;
    ScheduleGrid *m_grid;
};

#endif // ENTITYSCHEDULEDIALOG_H
