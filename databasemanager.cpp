#include "databasemanager.h"
#include <QFile>
#include <QFileInfo>

namespace {
const char *tableOf(Entity entity)
{
    switch (entity) {
    case Entity::Group: return "groups";
    case Entity::Teacher: return "teachers";
    case Entity::Classroom: return "classrooms";
    default: return "subjects";
    }
}

const char *scheduleColumnOf(Entity entity)
{
    switch (entity) {
    case Entity::Group: return "group_id";
    case Entity::Teacher: return "teacher_id";
    case Entity::Classroom: return "classroom_id";
    default: return "subject_id";
    }
}
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::connectToDatabase(const QString &dbPath) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);
    m_dbPath = dbPath;

    if (!m_db.open()) {
        qDebug() << "Помилка відкриття БД:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery(m_db).exec("PRAGMA foreign_keys = ON;");
    return createTables() && seedInitialData();
}

void DatabaseManager::closeDatabase() {
    if (!m_db.isValid()) return;
    const QString connection = m_db.connectionName();
    if (m_db.isOpen()) m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

bool DatabaseManager::createTables() {
    const QStringList statements = {
        R"(CREATE TABLE IF NOT EXISTS groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE);)",
        R"(CREATE TABLE IF NOT EXISTS teachers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            full_name TEXT NOT NULL,
            department TEXT);)",
        R"(CREATE TABLE IF NOT EXISTS classrooms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_number TEXT NOT NULL UNIQUE,
            capacity INTEGER DEFAULT 30);)",
        R"(CREATE TABLE IF NOT EXISTS subjects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            color_hex TEXT DEFAULT '#4CAF50');)",
        R"(CREATE TABLE IF NOT EXISTS bells (
            pair_number INTEGER PRIMARY KEY,
            start_time TEXT NOT NULL,
            end_time TEXT NOT NULL);)",
        R"(CREATE TABLE IF NOT EXISTS schedule (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            group_id INTEGER NOT NULL,
            subject_id INTEGER NOT NULL,
            teacher_id INTEGER NOT NULL,
            classroom_id INTEGER NOT NULL,
            day_of_week INTEGER NOT NULL CHECK(day_of_week BETWEEN 1 AND 5),
            pair_number INTEGER NOT NULL CHECK(pair_number > 0),
            week_type INTEGER NOT NULL DEFAULT 0 CHECK(week_type BETWEEN 0 AND 4),
            FOREIGN KEY(group_id) REFERENCES groups(id) ON DELETE CASCADE,
            FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE CASCADE,
            FOREIGN KEY(teacher_id) REFERENCES teachers(id) ON DELETE CASCADE,
            FOREIGN KEY(classroom_id) REFERENCES classrooms(id) ON DELETE CASCADE);)",
        "CREATE INDEX IF NOT EXISTS idx_schedule_slot ON schedule(day_of_week, pair_number);",
        "CREATE INDEX IF NOT EXISTS idx_schedule_group ON schedule(group_id);",
        "CREATE INDEX IF NOT EXISTS idx_schedule_teacher ON schedule(teacher_id);",
        "CREATE INDEX IF NOT EXISTS idx_schedule_room ON schedule(classroom_id);"
    };

    QSqlQuery query(m_db);
    for (const QString &sql : statements) {
        if (!query.exec(sql)) {
            qDebug() << "Помилка створення таблиці:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool DatabaseManager::seedInitialData() {
    QSqlQuery query(m_db);
    m_db.transaction();

    query.exec("SELECT COUNT(*) FROM groups;");
    if (query.next() && query.value(0).toInt() == 0) {
        const QStringList seed = {
            "INSERT INTO groups (name) VALUES ('КІ-106'), ('КН-223'), ('КІ-405');",
            "INSERT INTO teachers (full_name, department) VALUES ('Марцюк О. В.', 'Системного програмування'), ('Геник М. С.', 'Інформатики');",
            "INSERT INTO classrooms (room_number, capacity) VALUES ('201', 30), ('204', 40), ('312', 25);",
            "INSERT INTO subjects (name, color_hex) VALUES ('Системне програмування', '#4CAF50'), ('Основи C++', '#2196F3'), ('Бази даних', '#FF9800');"
        };
        for (const QString &sql : seed) {
            if (!query.exec(sql)) {
                m_db.rollback();
                return false;
            }
        }
    }

    query.exec("SELECT COUNT(*) FROM bells;");
    if (query.next() && query.value(0).toInt() == 0) {
        if (!query.exec("INSERT INTO bells (pair_number, start_time, end_time) VALUES "
                        "(1, '08:30', '09:50'), (2, '10:10', '11:30'), (3, '11:50', '13:10'), "
                        "(4, '13:30', '14:50'), (5, '15:05', '16:25');")) {
            m_db.rollback();
            return false;
        }
    }
    return m_db.commit();
}

QList<GroupItem> DatabaseManager::getGroups() {
    QList<GroupItem> list;
    QSqlQuery query("SELECT id, name FROM groups ORDER BY name;", m_db);
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString()});
    }
    return list;
}

QList<TeacherItem> DatabaseManager::getTeachers() {
    QList<TeacherItem> list;
    QSqlQuery query("SELECT id, full_name, department FROM teachers ORDER BY full_name;", m_db);
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()});
    }
    return list;
}

QList<ClassroomItem> DatabaseManager::getClassrooms() {
    QList<ClassroomItem> list;
    QSqlQuery query("SELECT id, room_number, capacity FROM classrooms ORDER BY room_number;", m_db);
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toInt()});
    }
    return list;
}

QList<SubjectItem> DatabaseManager::getSubjects() {
    QList<SubjectItem> list;
    QSqlQuery query("SELECT id, name, color_hex FROM subjects ORDER BY name;", m_db);
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()});
    }
    return list;
}

QList<BellItem> DatabaseManager::getBellSchedule() {
    QList<BellItem> list;
    QSqlQuery query("SELECT pair_number, start_time, end_time FROM bells ORDER BY pair_number;", m_db);
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()});
    }
    return list;
}

bool DatabaseManager::runSave(int id, const QString &insertSql, const QString &updateSql, const QVariantMap &values,
                              const QString &duplicateMessage, QString &outErrorMessage) {
    QSqlQuery query(m_db);
    query.prepare(id > 0 ? updateSql : insertSql);
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        query.bindValue(":" + it.key(), it.value());
    }
    if (id > 0) query.bindValue(":id", id);

    if (!query.exec()) {
        const QString text = query.lastError().text();
        outErrorMessage = text.contains("UNIQUE", Qt::CaseInsensitive)
                              ? duplicateMessage
                              : "Помилка бази даних: " + text;
        return false;
    }
    return true;
}

bool DatabaseManager::saveGroup(int id, const QString &name, QString &outErrorMessage) {
    if (name.trimmed().isEmpty()) {
        outErrorMessage = "Назва групи не може бути порожньою.";
        return false;
    }
    return runSave(id,
                   "INSERT INTO groups (name) VALUES (:name);",
                   "UPDATE groups SET name = :name WHERE id = :id;",
                   {{"name", name.trimmed()}},
                   "Група з такою назвою вже існує.", outErrorMessage);
}

bool DatabaseManager::saveTeacher(int id, const QString &fullName, const QString &department, QString &outErrorMessage) {
    if (fullName.trimmed().isEmpty()) {
        outErrorMessage = "ПІБ викладача не може бути порожнім.";
        return false;
    }
    return runSave(id,
                   "INSERT INTO teachers (full_name, department) VALUES (:name, :dept);",
                   "UPDATE teachers SET full_name = :name, department = :dept WHERE id = :id;",
                   {{"name", fullName.trimmed()}, {"dept", department.trimmed()}},
                   "Такий викладач вже існує.", outErrorMessage);
}

bool DatabaseManager::saveClassroom(int id, const QString &roomNumber, int capacity, QString &outErrorMessage) {
    if (roomNumber.trimmed().isEmpty()) {
        outErrorMessage = "Номер аудиторії не може бути порожнім.";
        return false;
    }
    return runSave(id,
                   "INSERT INTO classrooms (room_number, capacity) VALUES (:room, :cap);",
                   "UPDATE classrooms SET room_number = :room, capacity = :cap WHERE id = :id;",
                   {{"room", roomNumber.trimmed()}, {"cap", capacity > 0 ? capacity : 30}},
                   "Аудиторія з таким номером вже існує.", outErrorMessage);
}

bool DatabaseManager::saveSubject(int id, const QString &name, const QString &colorHex, QString &outErrorMessage) {
    if (name.trimmed().isEmpty()) {
        outErrorMessage = "Назва предмета не може бути порожньою.";
        return false;
    }
    const QString color = QColor::isValidColorName(colorHex) ? colorHex : QString("#4CAF50");
    return runSave(id,
                   "INSERT INTO subjects (name, color_hex) VALUES (:name, :color);",
                   "UPDATE subjects SET name = :name, color_hex = :color WHERE id = :id;",
                   {{"name", name.trimmed()}, {"color", color}},
                   "Такий предмет вже існує.", outErrorMessage);
}

bool DatabaseManager::deleteEntity(Entity entity, int id, QString &outErrorMessage) {
    if (id <= 0) {
        outErrorMessage = "Некоректний ідентифікатор запису.";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QString("DELETE FROM %1 WHERE id = :id;").arg(tableOf(entity)));
    query.bindValue(":id", id);

    if (!query.exec()) {
        outErrorMessage = "Помилка видалення: " + query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        outErrorMessage = "Запис не знайдено.";
        return false;
    }
    return true;
}

int DatabaseManager::countLessons(Entity entity, int id) {
    QSqlQuery query(m_db);
    query.prepare(QString("SELECT COUNT(*) FROM schedule WHERE %1 = :id;").arg(scheduleColumnOf(entity)));
    query.bindValue(":id", id);
    return (query.exec() && query.next()) ? query.value(0).toInt() : 0;
}

QList<ClassroomItem> DatabaseManager::getFreeClassrooms(int dayOfWeek, int pairNumber, int weekType, int excludeRecordId) {
    QList<ClassroomItem> list;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT c.id, c.room_number, c.capacity FROM classrooms c
        WHERE c.id NOT IN (
            SELECT s.classroom_id FROM schedule s
            WHERE s.id != :self AND s.day_of_week = :day AND s.pair_number = :pair
              AND (s.week_type = 0 OR :w_all = 0 OR s.week_type = :w_type))
        ORDER BY c.capacity, c.room_number;
    )");
    query.bindValue(":self", excludeRecordId);
    query.bindValue(":day", dayOfWeek);
    query.bindValue(":pair", pairNumber);
    query.bindValue(":w_all", weekType);
    query.bindValue(":w_type", weekType);

    if (query.exec()) {
        while (query.next()) {
            list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toInt()});
        }
    }
    return list;
}

bool DatabaseManager::updateBellSchedule(const QList<BellItem> &bells, QString &outErrorMessage) {
    if (bells.isEmpty()) {
        outErrorMessage = "Розклад дзвінків не може бути порожнім.";
        return false;
    }

    QSqlQuery query(m_db);
    query.exec("SELECT COALESCE(MAX(pair_number), 0) FROM schedule;");
    if (query.next() && query.value(0).toInt() > bells.size()) {
        outErrorMessage = QString("У розкладі вже є заняття на %1-й парі. Видалення пар, що використовуються, неможливе.")
                              .arg(query.value(0).toInt());
        return false;
    }

    m_db.transaction();
    if (!query.exec("DELETE FROM bells;")) {
        outErrorMessage = query.lastError().text();
        m_db.rollback();
        return false;
    }

    query.prepare("INSERT INTO bells (pair_number, start_time, end_time) VALUES (:p, :s, :e);");
    for (const auto &b : bells) {
        query.bindValue(":p", b.pairNumber);
        query.bindValue(":s", b.startTime);
        query.bindValue(":e", b.endTime);
        if (!query.exec()) {
            outErrorMessage = query.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        outErrorMessage = m_db.lastError().text();
        return false;
    }
    return true;
}

QString DatabaseManager::findConflict(const ScheduleRecord &record) {
    struct Check {
        const char *column;
        int value;
        const char *message;
    };

    const Check checks[] = {
        {"s.teacher_id", record.teacherId, "Конфлікт: викладач вже веде пару у групи %1 у цей час!"},
        {"s.classroom_id", record.classroomId, "Конфлікт: аудиторія вже зайнята групою %1 у цей час!"},
        {"s.group_id", record.groupId, "Конфлікт: у групи %1 вже є пара у цей час!"}
    };

    for (const auto &c : checks) {
        QSqlQuery query(m_db);
        query.prepare(QString("SELECT g.name FROM schedule s JOIN groups g ON s.group_id = g.id "
                              "WHERE %1 = :value AND s.id != :self AND s.day_of_week = :day AND s.pair_number = :pair "
                              "AND (s.week_type = 0 OR :w_all = 0 OR s.week_type = :w_type) LIMIT 1;")
                          .arg(c.column));
        query.bindValue(":value", c.value);
        query.bindValue(":self", record.id);
        query.bindValue(":day", record.dayOfWeek);
        query.bindValue(":pair", record.pairNumber);
        query.bindValue(":w_all", record.weekType);
        query.bindValue(":w_type", record.weekType);

        if (!query.exec())
            return "Помилка бази даних: " + query.lastError().text();
        if (query.next())
            return QString::fromUtf8(c.message).arg(query.value(0).toString());
    }
    return QString();
}

bool DatabaseManager::addScheduleRecord(const ScheduleRecord &record, QString &outErrorMessage) {
    if (record.groupId <= 0 || record.subjectId <= 0 || record.teacherId <= 0 || record.classroomId <= 0) {
        outErrorMessage = "Заповніть усі поля: група, предмет, викладач та аудиторія.";
        return false;
    }

    outErrorMessage = findConflict(record);
    if (!outErrorMessage.isEmpty())
        return false;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO schedule (group_id, subject_id, teacher_id, classroom_id, day_of_week, pair_number, week_type) "
                  "VALUES (:g_id, :s_id, :t_id, :c_id, :day, :pair, :w_type);");
    query.bindValue(":g_id", record.groupId);
    query.bindValue(":s_id", record.subjectId);
    query.bindValue(":t_id", record.teacherId);
    query.bindValue(":c_id", record.classroomId);
    query.bindValue(":day", record.dayOfWeek);
    query.bindValue(":pair", record.pairNumber);
    query.bindValue(":w_type", record.weekType);

    if (!query.exec()) {
        outErrorMessage = "Помилка бази даних: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::updateScheduleRecord(const ScheduleRecord &record, QString &outErrorMessage) {
    if (record.id <= 0 || record.groupId <= 0 || record.subjectId <= 0 || record.teacherId <= 0 || record.classroomId <= 0) {
        outErrorMessage = "Заповніть усі поля: предмет, викладач та аудиторія.";
        return false;
    }

    outErrorMessage = findConflict(record);
    if (!outErrorMessage.isEmpty())
        return false;

    QSqlQuery query(m_db);
    query.prepare("UPDATE schedule SET subject_id = :s_id, teacher_id = :t_id, classroom_id = :c_id, "
                  "day_of_week = :day, pair_number = :pair, week_type = :w_type WHERE id = :id;");
    query.bindValue(":s_id", record.subjectId);
    query.bindValue(":t_id", record.teacherId);
    query.bindValue(":c_id", record.classroomId);
    query.bindValue(":day", record.dayOfWeek);
    query.bindValue(":pair", record.pairNumber);
    query.bindValue(":w_type", record.weekType);
    query.bindValue(":id", record.id);

    if (!query.exec()) {
        outErrorMessage = "Помилка бази даних: " + query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::getScheduleRecord(int recordId, ScheduleRecord &outRecord) {
    QSqlQuery query(m_db);
    query.prepare("SELECT id, group_id, subject_id, teacher_id, classroom_id, day_of_week, pair_number, week_type "
                  "FROM schedule WHERE id = :id;");
    query.bindValue(":id", recordId);
    if (!query.exec() || !query.next())
        return false;

    outRecord.id = query.value(0).toInt();
    outRecord.groupId = query.value(1).toInt();
    outRecord.subjectId = query.value(2).toInt();
    outRecord.teacherId = query.value(3).toInt();
    outRecord.classroomId = query.value(4).toInt();
    outRecord.dayOfWeek = query.value(5).toInt();
    outRecord.pairNumber = query.value(6).toInt();
    outRecord.weekType = query.value(7).toInt();
    return true;
}

QList<ScheduleRecord> DatabaseManager::loadSchedule(const QString &filterColumn, int filterId, int weekType) {
    QList<ScheduleRecord> list;

    QString sql = R"SQL(
        SELECT s.id, s.group_id, s.subject_id, s.teacher_id, s.classroom_id,
               s.day_of_week, s.pair_number, s.week_type,
               sub.name, t.full_name, c.room_number, sub.color_hex, g.name
        FROM schedule s
        JOIN subjects sub ON s.subject_id = sub.id
        JOIN teachers t ON s.teacher_id = t.id
        JOIN classrooms c ON s.classroom_id = c.id
        JOIN groups g ON s.group_id = g.id
        WHERE (s.week_type = 0 OR :w_all = 0 OR s.week_type = :w_type)
    )SQL";
    if (!filterColumn.isEmpty()) {
        sql += QString(" AND %1 = :filter").arg(filterColumn);
    }
    sql += " ORDER BY s.day_of_week, s.pair_number, s.week_type;";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":w_all", weekType);
    query.bindValue(":w_type", weekType);
    if (!filterColumn.isEmpty()) query.bindValue(":filter", filterId);

    if (!query.exec()) {
        qDebug() << "Помилка завантаження розкладу:" << query.lastError().text();
        return list;
    }

    while (query.next()) {
        ScheduleRecord r;
        r.id = query.value(0).toInt();
        r.groupId = query.value(1).toInt();
        r.subjectId = query.value(2).toInt();
        r.teacherId = query.value(3).toInt();
        r.classroomId = query.value(4).toInt();
        r.dayOfWeek = query.value(5).toInt();
        r.pairNumber = query.value(6).toInt();
        r.weekType = query.value(7).toInt();
        r.subjectName = query.value(8).toString();
        r.teacherName = query.value(9).toString();
        r.roomNumber = query.value(10).toString();
        r.colorHex = query.value(11).toString();
        r.groupName = query.value(12).toString();
        list.append(r);
    }
    return list;
}

QList<ScheduleRecord> DatabaseManager::getScheduleForEntity(Entity entity, int id, int weekType) {
    switch (entity) {
    case Entity::Group: return loadSchedule("s.group_id", id, weekType);
    case Entity::Teacher: return loadSchedule("s.teacher_id", id, weekType);
    case Entity::Classroom: return loadSchedule("s.classroom_id", id, weekType);
    default: return loadSchedule("s.subject_id", id, weekType);
    }
}

QList<ScheduleRecord> DatabaseManager::getScheduleForGroup(int groupId, int weekType) {
    return loadSchedule("s.group_id", groupId, weekType);
}

QList<ScheduleRecord> DatabaseManager::getScheduleForTeacher(int teacherId, int weekType) {
    return loadSchedule("s.teacher_id", teacherId, weekType);
}

QList<ScheduleRecord> DatabaseManager::getScheduleForClassroom(int classroomId, int weekType) {
    return loadSchedule("s.classroom_id", classroomId, weekType);
}

QList<ScheduleRecord> DatabaseManager::getAllScheduleRecords() {
    return loadSchedule(QString(), 0, 0);
}

bool DatabaseManager::deleteScheduleForGroup(int groupId) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM schedule WHERE group_id = :id;");
    query.bindValue(":id", groupId);
    return query.exec();
}

bool DatabaseManager::beginTransaction() {
    return m_db.transaction();
}

bool DatabaseManager::commitTransaction() {
    return m_db.commit();
}

void DatabaseManager::rollbackTransaction() {
    m_db.rollback();
}

bool DatabaseManager::deleteScheduleRecord(int recordId) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM schedule WHERE id = :id;");
    query.bindValue(":id", recordId);
    return query.exec() && query.numRowsAffected() > 0;
}

bool DatabaseManager::backupTo(const QString &path, QString &outErrorMessage) {
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(m_dbPath).canonicalFilePath() && QFileInfo::exists(path)) {
        outErrorMessage = "Не можна зберегти копію поверх робочої бази.";
        return false;
    }

    QFile::remove(path);

    QString escaped = path;
    escaped.replace("'", "''");
    QSqlQuery query(m_db);
    if (query.exec(QString("VACUUM INTO '%1';").arg(escaped))) {
        return true;
    }

    const QString vacuumError = query.lastError().text();
    if (QFile::copy(m_dbPath, path)) {
        return true;
    }

    outErrorMessage = "Не вдалося створити копію: " + vacuumError;
    return false;
}

bool DatabaseManager::isValidBackup(const QString &path) {
    if (!QFileInfo::exists(path)) return false;

    const QString connectionName = "smartschedule_backup_check";
    bool valid = false;
    {
        QSqlDatabase check = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        check.setDatabaseName(path);
        if (check.open()) {
            const QStringList tables = check.tables();
            valid = tables.contains("groups") && tables.contains("teachers") && tables.contains("classrooms")
                    && tables.contains("subjects") && tables.contains("bells") && tables.contains("schedule");
            check.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);
    return valid;
}

bool DatabaseManager::restoreFrom(const QString &path, QString &outErrorMessage) {
    if (!isValidBackup(path)) {
        outErrorMessage = "Файл не є коректною копією бази SmartSchedule.";
        return false;
    }
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(m_dbPath).canonicalFilePath()) {
        outErrorMessage = "Оберіть файл, відмінний від поточної бази.";
        return false;
    }

    const QString currentPath = m_dbPath;
    const QString safetyPath = currentPath + ".before-restore";

    closeDatabase();

    QFile::remove(safetyPath);
    const bool saved = QFile::copy(currentPath, safetyPath);

    bool replaced = QFile::remove(currentPath) && QFile::copy(path, currentPath);
    if (replaced) {
        QFile::setPermissions(currentPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    } else {
        outErrorMessage = "Не вдалося замінити файл бази даних.";
        if (saved) {
            QFile::remove(currentPath);
            QFile::copy(safetyPath, currentPath);
        }
    }

    if (!connectToDatabase(currentPath)) {
        outErrorMessage = "Не вдалося відкрити базу даних після відновлення.";
        return false;
    }
    return replaced;
}
