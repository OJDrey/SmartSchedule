PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS groups (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE CHECK(length(trim(name)) BETWEEN 1 AND 50)
);

CREATE TABLE IF NOT EXISTS teachers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    full_name TEXT NOT NULL CHECK(length(trim(full_name)) BETWEEN 1 AND 100),
    department TEXT
);

CREATE TABLE IF NOT EXISTS classrooms (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_number TEXT NOT NULL UNIQUE,
    capacity INTEGER NOT NULL DEFAULT 30 CHECK(capacity > 0)
);

CREATE TABLE IF NOT EXISTS subjects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL CHECK(length(trim(name)) BETWEEN 1 AND 100),
    color_hex TEXT DEFAULT '#4CAF50'
);

CREATE TABLE IF NOT EXISTS bells (
    pair_number INTEGER PRIMARY KEY,
    start_time TEXT NOT NULL,
    end_time TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS schedule (
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
    FOREIGN KEY(classroom_id) REFERENCES classrooms(id) ON DELETE CASCADE
);

CREATE UNIQUE INDEX IF NOT EXISTS uq_teachers_name ON teachers(full_name);
CREATE UNIQUE INDEX IF NOT EXISTS uq_subjects_name ON subjects(name);
CREATE UNIQUE INDEX IF NOT EXISTS uq_schedule_cell ON schedule(group_id, day_of_week, pair_number, week_type);
CREATE INDEX IF NOT EXISTS idx_schedule_slot ON schedule(day_of_week, pair_number);
CREATE INDEX IF NOT EXISTS idx_schedule_group ON schedule(group_id);
CREATE INDEX IF NOT EXISTS idx_schedule_teacher ON schedule(teacher_id);
CREATE INDEX IF NOT EXISTS idx_schedule_room ON schedule(classroom_id);

INSERT INTO groups (name) SELECT 'КІ-106' WHERE NOT EXISTS (SELECT 1 FROM groups);
INSERT INTO bells (pair_number, start_time, end_time)
SELECT * FROM (SELECT 1, '08:30', '09:50' UNION ALL SELECT 2, '10:10', '11:30' UNION ALL SELECT 3, '11:50', '13:10'
               UNION ALL SELECT 4, '13:30', '14:50' UNION ALL SELECT 5, '15:05', '16:25')
WHERE NOT EXISTS (SELECT 1 FROM bells);
