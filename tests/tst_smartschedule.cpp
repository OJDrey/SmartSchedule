#include <QtTest>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QFile>
#include "databasemanager.h"
#include "scheduleimporter.h"
#include "scheduleanalysis.h"

class TstSmartSchedule : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;
    QString m_path;
    DatabaseManager &db = DatabaseManager::instance();

    ScheduleRecord rec(int g, int t, int c, int day, int pair, int week = 0) {
        ScheduleRecord r;
        r.groupId = g; r.subjectId = 1; r.teacherId = t; r.classroomId = c;
        r.dayOfWeek = day; r.pairNumber = pair; r.weekType = week;
        return r;
    }
    int count(const QString &sql) {
        QSqlQuery q(QSqlDatabase::database());
        q.exec(sql);
        return q.next() ? q.value(0).toInt() : -1;
    }

private slots:
    void init() {
        m_path = m_dir.filePath(QString("t%1.db").arg(QTest::currentTestFunction()));
        QFile::remove(m_path);
        QVERIFY(db.connectToDatabase(m_path));
    }
    void cleanup() { db.closeDatabase(); }

    void T01_foreignKeysEnabled() {
        QSqlQuery q(QSqlDatabase::database());
        QVERIFY(q.exec("PRAGMA foreign_keys"));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
    }
    void T02_addPersistsAfterReopen() {
        QString err;
        QVERIFY2(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err), qPrintable(err));
        db.closeDatabase();
        QVERIFY(db.connectToDatabase(m_path));
        QCOMPARE(db.getScheduleForGroup(1).size(), 1);
    }
    void T03_teacherConflict() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        QVERIFY(!db.addScheduleRecord(rec(2, 1, 2, 1, 1), err));
        QVERIFY(err.contains("викладач"));
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 1);
    }
    void T04_classroomConflict() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        QVERIFY(!db.addScheduleRecord(rec(2, 2, 1, 1, 1), err));
        QVERIFY(err.contains("аудиторія"));
    }
    void T05_groupConflict() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        QVERIFY(!db.addScheduleRecord(rec(1, 2, 2, 1, 1), err));
        QVERIFY(err.contains("групи"));
    }
    void T06_differentWeeksNoConflict() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1, 1), err));
        QVERIFY2(db.addScheduleRecord(rec(1, 1, 1, 1, 1, 2), err), qPrintable(err));
        QVERIFY(!db.addScheduleRecord(rec(2, 1, 2, 1, 1, 0), err));
    }
    void T07_updateDoesNotConflictWithItself() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        QVERIFY(db.addScheduleRecord(rec(2, 2, 2, 2, 2), err));
        ScheduleRecord r = db.getScheduleForGroup(1).first();
        r.classroomId = 3;
        QVERIFY2(db.updateScheduleRecord(r, err), qPrintable(err));
        QCOMPARE(db.getScheduleForGroup(1).first().classroomId, 3);
        r.dayOfWeek = 2; r.pairNumber = 2; r.teacherId = 2;
        QVERIFY(db.updateScheduleRecord(r, err) == false);
    }
    void T08_deleteMissingRecordFails() {
        QVERIFY(!db.deleteScheduleRecord(9999));
    }
    void T09_cascadeDeleteAndCount() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        QCOMPARE(db.countLessons(Entity::Teacher, 1), 1);
        QVERIFY(db.deleteEntity(Entity::Teacher, 1, err));
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 0);
        QVERIFY(!db.deleteEntity(Entity::Teacher, 1, err));
    }
    void T10_uniqueNames() {
        QString err;
        QVERIFY(!db.saveTeacher(0, "Марцюк О. В.", "X", err));
        QVERIFY(err.contains("вже існує"));
        QVERIFY(!db.saveGroup(0, "КІ-106", err));
        QVERIFY(!db.saveSubject(0, "Бази даних", "#FFFFFF", err));
        QVERIFY(!db.saveClassroom(0, "201", 10, err));
    }
    void T11_emptyAndInvalidInput() {
        QString err;
        QVERIFY(!db.saveGroup(0, "   ", err));
        QVERIFY(!db.saveTeacher(0, "", "", err));
        QVERIFY(!db.addScheduleRecord(rec(0, 1, 1, 1, 1), err));
        QVERIFY(!db.deleteEntity(Entity::Group, 0, err));
    }
    void T12_sqlInjectionTreatedAsData() {
        QString err;
        const QString evil = "x'); DROP TABLE schedule;--";
        QVERIFY2(db.saveGroup(0, evil, err), qPrintable(err));
        QVERIFY(count("SELECT COUNT(*) FROM groups WHERE name = '" + QString(evil).replace("'", "''") + "'") == 1);
        QVERIFY(count("SELECT COUNT(*) FROM schedule") >= 0);
    }
    void T13_rollbackDiscardsChanges() {
        QString err;
        QVERIFY(db.beginTransaction());
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 1);
        db.rollbackTransaction();
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 0);
        QVERIFY(db.beginTransaction());
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        QVERIFY(db.commitTransaction());
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 1);
    }
    void T14_bellsRefusedWhenInUse() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 5), err));
        QList<BellItem> bells = db.getBellSchedule();
        bells.removeLast();
        QVERIFY(!db.updateBellSchedule(bells, err));
        QCOMPARE(db.getBellSchedule().size(), 5);
        bells = db.getBellSchedule();
        bells.append({6, "16:40", "18:00"});
        QVERIFY2(db.updateBellSchedule(bells, err), qPrintable(err));
        QCOMPARE(db.getBellSchedule().size(), 6);
    }
    void T15_constraintsRejectRawInserts() {
        QSqlQuery q(QSqlDatabase::database());
        QVERIFY(!q.exec("INSERT INTO schedule(group_id,subject_id,teacher_id,classroom_id,day_of_week,pair_number)"
                        " VALUES(1,1,1,1,9,1)"));
        QVERIFY(!q.exec("INSERT INTO schedule(group_id,subject_id,teacher_id,classroom_id,day_of_week,pair_number)"
                        " VALUES(777,1,1,1,1,1)"));
        QVERIFY(!q.exec("INSERT INTO classrooms(room_number,capacity) VALUES('X1',-5)"));
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 0);
    }
    void T16_freeClassrooms() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        const auto free = db.getFreeClassrooms(1, 1, 0);
        QCOMPARE(free.size(), 2);
        for (const auto &c : free) QVERIFY(c.id != 1);
    }
    void T17_analyzerFindsConflictsAndWindows() {
        QList<ScheduleRecord> list = {rec(1, 1, 1, 1, 1), rec(2, 1, 2, 1, 1), rec(1, 2, 3, 1, 4)};
        const auto conflicts = ScheduleAnalyzer::findConflicts(list, db.getBellSchedule());
        QCOMPARE(conflicts.size(), 1);
        QCOMPARE(conflicts.first().type, QString("Викладач"));
        QList<ScheduleRecord> one = {rec(1, 1, 1, 1, 1), rec(1, 1, 1, 1, 4)};
        QCOMPARE(ScheduleAnalyzer::findWindows(one, 0).size(), 2);
        QCOMPARE(ScheduleAnalyzer::findWindows({}, 0).size(), 0);
    }
    void T18_importParsesAndReportsErrors() {
        QList<QStringList> rows = {
            ScheduleImporter::header(),
            {"КІ-106", "Понеділок", "1", "Постійно", "Бази даних", "Марцюк О. В.", "201"},
            {"КІ-106", "Субота", "1", "Постійно", "Бази даних", "Марцюк О. В.", "201"},
            {"КІ-106", "2", "x", "Постійно", "Бази даних", "Марцюк О. В.", "201"},
            {"НОВА", "3", "2", "1-й тиждень (Чисельник)", "Новий предмет", "Новий В. В.", "999"}
        };
        auto parsed = ScheduleImporter::parse(rows);
        QCOMPARE(parsed.size(), 4);
        QVERIFY(parsed[1].error.contains("день"));
        QVERIFY(parsed[2].error.contains("пари"));
        const auto report = ScheduleImporter::run(parsed, ImportOptions{false, false});
        QCOMPARE(report.total, 4);
        QCOMPARE(report.imported, 1);
        QCOMPARE(report.errors.size(), 3);
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 1);
    }
    void T19_importCreatesMissingAndReplaces() {
        QList<QStringList> rows = {
            {"НОВА", "3", "2", "1", "Новий предмет", "Новий В. В.", "999"}
        };
        auto parsed = ScheduleImporter::parse(rows);
        QCOMPARE(ScheduleImporter::run(parsed, ImportOptions{false, true}).imported, 1);
        QCOMPARE(count("SELECT COUNT(*) FROM groups WHERE name='НОВА'"), 1);
        parsed = ScheduleImporter::parse(rows);
        QCOMPARE(ScheduleImporter::run(parsed, ImportOptions{true, true}).imported, 1);
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 1);
        QCOMPARE(count("SELECT COUNT(*) FROM teachers WHERE full_name='Новий В. В.'"), 1);
    }
    void T20_backupAndRestore() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        const QString bak = m_dir.filePath("backup.db");
        QVERIFY2(db.backupTo(bak, err), qPrintable(err));
        QVERIFY(db.deleteScheduleRecord(db.getScheduleForGroup(1).first().id));
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 0);
        QVERIFY2(db.restoreFrom(bak, err), qPrintable(err));
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 1);
        QFile bad(m_dir.filePath("bad.db"));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("not a database");
        bad.close();
        QVERIFY(!db.restoreFrom(bad.fileName(), err));
    }
    void T21_importAbortsWhenReplaceFails() {
        QString err;
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        QSqlQuery q(QSqlDatabase::database());
        QVERIFY(q.exec("CREATE TRIGGER block_delete BEFORE DELETE ON schedule BEGIN SELECT RAISE(ABORT,'blocked'); END"));
        QList<QStringList> rows = {{"КІ-106", "2", "2", "0", "Бази даних", "Геник М. С.", "204"}};
        auto parsed = ScheduleImporter::parse(rows);
        const auto report = ScheduleImporter::run(parsed, ImportOptions{true, false});
        QCOMPARE(report.imported, 0);
        QVERIFY(!report.errors.isEmpty());
        QVERIFY(q.exec("DROP TRIGGER block_delete"));
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 1);
        QCOMPARE(count("SELECT COUNT(*) FROM schedule WHERE day_of_week=2"), 0);
    }
    void T22_importReportsCreateErrors() {
        const QString longName(60, QChar(u'Ж'));
        QList<QStringList> rows = {{longName, "2", "2", "0", "Бази даних", "Геник М. С.", "204"}};
        auto parsed = ScheduleImporter::parse(rows);
        const auto report = ScheduleImporter::run(parsed, ImportOptions{false, true});
        QCOMPARE(report.imported, 0);
        QVERIFY(report.errors.join("\n").contains("Не вдалося створити групу"));
    }
    void T23_backupOverwritesAndKeepsOldOnFailure() {
        QString err;
        const QString bak = m_dir.filePath("keep.db");
        QVERIFY2(db.backupTo(bak, err), qPrintable(err));
        QVERIFY(db.addScheduleRecord(rec(1, 1, 1, 1, 1), err));
        QVERIFY2(db.backupTo(bak, err), qPrintable(err));
        QVERIFY(!QFile::exists(bak + ".tmp"));
        QVERIFY(!db.backupTo(m_dir.filePath("no/such/dir/x.db"), err));
        QVERIFY(QFile::exists(bak));
        QVERIFY2(db.restoreFrom(bak, err), qPrintable(err));
        QCOMPARE(count("SELECT COUNT(*) FROM schedule"), 1);
    }
    void T24_reopenAfterDeletingAllGroups() {
        QString err;
        for (const auto &g : db.getGroups()) QVERIFY2(db.deleteEntity(Entity::Group, g.id, err), qPrintable(err));
        db.closeDatabase();
        QVERIFY(db.connectToDatabase(m_path));
        QCOMPARE(db.getGroups().size(), 0);
        QCOMPARE(db.getTeachers().size(), 2);
        QCOMPARE(db.getBellSchedule().size(), 5);
    }
};

QTEST_GUILESS_MAIN(TstSmartSchedule)
#include "tst_smartschedule.moc"
