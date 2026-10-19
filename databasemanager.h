#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QList>

struct GroupItem {
    int id;
    QString name;
};

struct TeacherItem {
    int id;
    QString name;
    QString department;
};

struct ClassroomItem {
    int id;
    QString roomNumber;
    int capacity;
};

struct SubjectItem {
    int id;
    QString name;
    QString colorHex;
};

struct BellItem {
    int pairNumber;
    QString startTime;
    QString endTime;
};

struct ScheduleRecord {
    int id;
    int groupId;
    int subjectId;
    int teacherId;
    int classroomId;
    int dayOfWeek;    // 1..5 (Пн..Пт)
    int pairNumber;   // 1..N пара
    int weekType;     // 0 = щотижня, 1 = чисельник, 2 = знаменник, 3 = 3 тиждень, 4 = 4 тиждень

    // Допоміжні поля для виводу у таблиці
    QString groupName;
    QString subjectName;
    QString teacherName;
    QString roomNumber;
    QString colorHex;
};

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool connectToDatabase(const QString &dbName = "schedule.db");
    bool createTables();
    void closeDatabase();

    // Отримання списків для ComboBox та таблиць
    QList<GroupItem> getGroups();
    QList<TeacherItem> getTeachers();
    QList<ClassroomItem> getClassrooms();
    QList<SubjectItem> getSubjects();
    QList<BellItem> getBellSchedule();

    // Оновлення розкладу дзвінків
    bool updateBellSchedule(const QList<BellItem> &bells);

    // Заповнення початковими даними при першому запуску
    void seedInitialData();

    // Додавання розкладу з автоперевіркою на конфлікти
    bool addScheduleRecord(const ScheduleRecord &record, QString &outErrorMessage);

    // Завантаження розкладу для конкретної групи
    QList<ScheduleRecord> getScheduleForGroup(int groupId, int weekType = 0);

    // Видалення пари
    bool deleteScheduleRecord(int recordId);

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H