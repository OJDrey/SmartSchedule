#include "databasemanager.h"

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::connectToDatabase(const QString &dbName) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbName);

    if (!m_db.open()) {
        qDebug() << "Помилка відкриття БД:" << m_db.lastError().text();
        return false;
    }

    qDebug() << "БД успішно відкрита:" << dbName;
    if (createTables()) {
        seedInitialData();
        return true;
    }
    return false;
}

void DatabaseManager::closeDatabase() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::createTables() {
    QSqlQuery query;

    QString createGroupsTable = R"(
        CREATE TABLE IF NOT EXISTS groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        );
    )";

    QString createTeachersTable = R"(
        CREATE TABLE IF NOT EXISTS teachers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            full_name TEXT NOT NULL,
            department TEXT
        );
    )";

    QString createRoomsTable = R"(
        CREATE TABLE IF NOT EXISTS classrooms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_number TEXT NOT NULL UNIQUE,
            capacity INTEGER DEFAULT 30
        );
    )";

    QString createSubjectsTable = R"(
        CREATE TABLE IF NOT EXISTS subjects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            color_hex TEXT DEFAULT '#4CAF50'
        );
    )";

    QString createBellsTable = R"(
        CREATE TABLE IF NOT EXISTS bells (
            pair_number INTEGER PRIMARY KEY,
            start_time TEXT NOT NULL,
            end_time TEXT NOT NULL
        );
    )";

    QString createScheduleTable = R"(
        CREATE TABLE IF NOT EXISTS schedule (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            group_id INTEGER NOT NULL,
            subject_id INTEGER NOT NULL,
            teacher_id INTEGER NOT NULL,
            classroom_id INTEGER NOT NULL,
            day_of_week INTEGER NOT NULL,
            pair_number INTEGER NOT NULL,
            week_type INTEGER DEFAULT 0,
            FOREIGN KEY(group_id) REFERENCES groups(id) ON DELETE CASCADE,
            FOREIGN KEY(subject_id) REFERENCES subjects(id) ON DELETE CASCADE,
            FOREIGN KEY(teacher_id) REFERENCES teachers(id) ON DELETE CASCADE,
            FOREIGN KEY(classroom_id) REFERENCES classrooms(id) ON DELETE CASCADE
        );
    )";

    QList<QString> tables = {
        createGroupsTable, createTeachersTable, createRoomsTable,
        createSubjectsTable, createBellsTable, createScheduleTable
    };

    for (const QString &sql : tables) {
        if (!query.exec(sql)) {
            qDebug() << "Помилка створення таблиці:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

void DatabaseManager::seedInitialData() {
    QSqlQuery query;
    query.exec("SELECT COUNT(*) FROM groups;");
    if (query.next() && query.value(0).toInt() == 0) {
        query.exec("INSERT INTO groups (name) VALUES ('КІ-106'), ('КН-223'), ('КІ-405');");
        query.exec("INSERT INTO teachers (full_name, department) VALUES ('Марцюк О. В.', 'Системного програмування'), ('Геник М. С.', 'Інформатики');");
        query.exec("INSERT INTO classrooms (room_number, capacity) VALUES ('201', 30), ('204', 40), ('312', 25);");
        query.exec("INSERT INTO subjects (name, color_hex) VALUES ('Системне програмування', '#4CAF50'), ('Основи C++', '#2196F3'), ('Бази даних', '#FF9800');");
    }

    query.exec("SELECT COUNT(*) FROM bells;");
    if (query.next() && query.value(0).toInt() == 0) {
        query.exec("INSERT INTO bells (pair_number, start_time, end_time) VALUES "
                   "(1, '08:30', '09:50'), "
                   "(2, '10:10', '11:30'), "
                   "(3, '11:50', '13:10'), "
                   "(4, '13:30', '14:50'), "
                   "(5, '15:05', '16:25');");
    }
}

QList<GroupItem> DatabaseManager::getGroups() {
    QList<GroupItem> list;
    QSqlQuery query("SELECT id, name FROM groups ORDER BY name;");
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString()});
    }
    return list;
}

QList<TeacherItem> DatabaseManager::getTeachers() {
    QList<TeacherItem> list;
    QSqlQuery query("SELECT id, full_name, department FROM teachers ORDER BY full_name;");
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()});
    }
    return list;
}

QList<ClassroomItem> DatabaseManager::getClassrooms() {
    QList<ClassroomItem> list;
    QSqlQuery query("SELECT id, room_number, capacity FROM classrooms ORDER BY room_number;");
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toInt()});
    }
    return list;
}

QList<SubjectItem> DatabaseManager::getSubjects() {
    QList<SubjectItem> list;
    QSqlQuery query("SELECT id, name, color_hex FROM subjects ORDER BY name;");
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()});
    }
    return list;
}

QList<BellItem> DatabaseManager::getBellSchedule() {
    QList<BellItem> list;
    QSqlQuery query("SELECT pair_number, start_time, end_time FROM bells ORDER BY pair_number;");
    while (query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()});
    }
    return list;
}

bool DatabaseManager::updateBellSchedule(const QList<BellItem> &bells) {
    QSqlQuery query;
    m_db.transaction();
    query.exec("DELETE FROM bells;");

    for (const auto &b : bells) {
        query.prepare("INSERT INTO bells (pair_number, start_time, end_time) VALUES (:p, :s, :e);");
        query.bindValue(":p", b.pairNumber);
        query.bindValue(":s", b.startTime);
        query.bindValue(":e", b.endTime);
        if (!query.exec()) {
            m_db.rollback();
            return false;
        }
    }
    return m_db.commit();
}

bool DatabaseManager::addScheduleRecord(const ScheduleRecord &record, QString &outErrorMessage) {
    QSqlQuery checkQuery;

    // Перевірка 1: Накладка викладача
    checkQuery.prepare("SELECT g.name FROM schedule s JOIN groups g ON s.group_id = g.id "
                       "WHERE s.teacher_id = :t_id AND s.day_of_week = :day AND s.pair_number = :pair "
                       "AND (s.week_type = 0 OR :w_type = 0 OR s.week_type = :w_type);");
    checkQuery.bindValue(":t_id", record.teacherId);
    checkQuery.bindValue(":day", record.dayOfWeek);
    checkQuery.bindValue(":pair", record.pairNumber);
    checkQuery.bindValue(":w_type", record.weekType);
    if (checkQuery.exec() && checkQuery.next()) {
        outErrorMessage = QString("Конфлікт: Викладач вже веде пару у групи %1 у цей час!").arg(checkQuery.value(0).toString());
        return false;
    }

    // Перевірка 2: Накладка аудиторії
    checkQuery.prepare("SELECT g.name FROM schedule s JOIN groups g ON s.group_id = g.id "
                       "WHERE s.classroom_id = :c_id AND s.day_of_week = :day AND s.pair_number = :pair "
                       "AND (s.week_type = 0 OR :w_type = 0 OR s.week_type = :w_type);");
    checkQuery.bindValue(":c_id", record.classroomId);
    checkQuery.bindValue(":day", record.dayOfWeek);
    checkQuery.bindValue(":pair", record.pairNumber);
    checkQuery.bindValue(":w_type", record.weekType);
    if (checkQuery.exec() && checkQuery.next()) {
        outErrorMessage = QString("Конфлікт: Аудиторія вже зайнята групою %1 у цей час!").arg(checkQuery.value(0).toString());
        return false;
    }

    // Додавання запису
    QSqlQuery insertQuery;
    insertQuery.prepare("INSERT INTO schedule (group_id, subject_id, teacher_id, classroom_id, day_of_week, pair_number, week_type) "
                        "VALUES (:g_id, :s_id, :t_id, :c_id, :day, :pair, :w_type);");
    insertQuery.bindValue(":g_id", record.groupId);
    insertQuery.bindValue(":s_id", record.subjectId);
    insertQuery.bindValue(":t_id", record.teacherId);
    insertQuery.bindValue(":c_id", record.classroomId);
    insertQuery.bindValue(":day", record.dayOfWeek);
    insertQuery.bindValue(":pair", record.pairNumber);
    insertQuery.bindValue(":w_type", record.weekType);

    if (!insertQuery.exec()) {
        outErrorMessage = "Помилка бази даних: " + insertQuery.lastError().text();
        return false;
    }
    return true;
}

QList<ScheduleRecord> DatabaseManager::getScheduleForGroup(int groupId, int weekType) {
    QList<ScheduleRecord> list;
    QSqlQuery query;
    query.prepare(R"(
        SELECT s.id, s.group_id, s.subject_id, s.teacher_id, s.classroom_id,
               s.day_of_week, s.pair_number, s.week_type,
               sub.name, t.full_name, c.room_number, sub.color_hex
        FROM schedule s
        JOIN subjects sub ON s.subject_id = sub.id
        JOIN teachers t ON s.teacher_id = t.id
        JOIN classrooms c ON s.classroom_id = c.id
        WHERE s.group_id = :g_id AND (s.week_type = 0 OR :w_type = 0 OR s.week_type = :w_type);
    )");
    query.bindValue(":g_id", groupId);
    query.bindValue(":w_type", weekType);

    if (query.exec()) {
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

            list.append(r);
        }
    }
    return list;
}

bool DatabaseManager::deleteScheduleRecord(int recordId) {
    QSqlQuery query;
    query.prepare("DELETE FROM schedule WHERE id = :id;");
    query.bindValue(":id", recordId);
    return query.exec();
}