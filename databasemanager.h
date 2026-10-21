#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <QList>
#include <QVariantMap>
#include <QColor>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

inline constexpr QChar kNewLine(QChar::LineFeed);

struct GroupItem {
    int id = 0;
    QString name;
};

struct TeacherItem {
    int id = 0;
    QString name;
    QString department;
};

struct ClassroomItem {
    int id = 0;
    QString roomNumber;
    int capacity = 0;
};

struct SubjectItem {
    int id = 0;
    QString name;
    QString colorHex;
};

struct BellItem {
    int pairNumber = 0;
    QString startTime;
    QString endTime;
};

struct ScheduleRecord {
    int id = 0;
    int groupId = 0;
    int subjectId = 0;
    int teacherId = 0;
    int classroomId = 0;
    int dayOfWeek = 0;
    int pairNumber = 0;
    int weekType = 0;

    QString subjectName;
    QString teacherName;
    QString roomNumber;
    QString colorHex;
};

enum class Entity { Group, Teacher, Classroom, Subject };

inline QString weekTypeName(int weekType)
{
    switch (weekType) {
    case 0: return "Постійно (Кожного тижня)";
    case 1: return "1-й тиждень (Чисельник)";
    case 2: return "2-й тиждень (Знаменник)";
    default: return QString("%1-й тиждень").arg(weekType);
    }
}

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool connectToDatabase(const QString &dbPath);
    void closeDatabase();

    QList<GroupItem> getGroups();
    QList<TeacherItem> getTeachers();
    QList<ClassroomItem> getClassrooms();
    QList<SubjectItem> getSubjects();
    QList<BellItem> getBellSchedule();

    bool saveGroup(int id, const QString &name, QString &outErrorMessage);
    bool saveTeacher(int id, const QString &fullName, const QString &department, QString &outErrorMessage);
    bool saveClassroom(int id, const QString &roomNumber, int capacity, QString &outErrorMessage);
    bool saveSubject(int id, const QString &name, const QString &colorHex, QString &outErrorMessage);
    bool deleteEntity(Entity entity, int id, QString &outErrorMessage);
    int countLessons(Entity entity, int id);

    bool updateBellSchedule(const QList<BellItem> &bells, QString &outErrorMessage);
    bool addScheduleRecord(const ScheduleRecord &record, QString &outErrorMessage);
    QList<ScheduleRecord> getScheduleForGroup(int groupId, int weekType = 0);
    bool deleteScheduleRecord(int recordId);

private:
    DatabaseManager() = default;
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool createTables();
    bool seedInitialData();
    bool runSave(int id, const QString &insertSql, const QString &updateSql, const QVariantMap &values,
                 const QString &duplicateMessage, QString &outErrorMessage);
    QString findConflict(const ScheduleRecord &record);

    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H