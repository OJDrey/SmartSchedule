#ifndef REFERENCEDIALOG_H
#define REFERENCEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QStringList>
#include <functional>
#include "databasemanager.h"

struct ReferenceField {
    enum Type { Text, Number, Color };
    QString label;
    Type type = Text;
};

struct ReferenceRow {
    int id = 0;
    QStringList values;
};

struct ReferenceConfig {
    QString title;
    QList<ReferenceField> fields;
    std::function<QList<ReferenceRow>()> load;
    std::function<bool(int, const QStringList &, QString &)> save;
    std::function<bool(int, QString &)> remove;
    std::function<int(int)> usage;
};

class ReferenceDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReferenceDialog(const ReferenceConfig &config, QWidget *parent = nullptr);

private slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();

private:
    void reload();
    int currentId() const;
    QStringList currentValues() const;
    bool runEditor(int id, const QStringList &initial);

    ReferenceConfig m_config;
    QTableWidget *m_tableWidget;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_closeBtn;
};

#endif // REFERENCEDIALOG_H