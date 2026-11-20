
#include "databasemanager.h"

#include <QDir>
#include <QDebug>
#include <QList>
#include <QPair>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>

// ==========================================
// Database Connection
// ==========================================

QString DatabaseManager::connectionName()
{
    return QStringLiteral("student_management_connection");
}

QSqlDatabase DatabaseManager::database()
{
    return QSqlDatabase::database(connectionName());
}

// ==========================================
// Database Initialization
// ==========================================

bool DatabaseManager::initialize(QString *errorMessage)
{
    // Get application data directory
    const QString dataPath =
        QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation
            );

    if (dataPath.isEmpty()) {
        if (errorMessage)
            *errorMessage =
                "Could not locate application data directory.";

        return false;
    }

    // Create directory
    QDir appDir(dataPath);

    if (!appDir.exists() && !QDir().mkpath(dataPath)) {
        if (errorMessage)
            *errorMessage =
                "Could not create database directory.";

        return false;
    }

    // Database path
    const QString databasePath =
        appDir.filePath("student_management.db");

    // Create or reuse connection
    QSqlDatabase db;

    if (QSqlDatabase::contains(connectionName())) {
        db = QSqlDatabase::database(connectionName());
    }
    else {
        db = QSqlDatabase::addDatabase(
            "QSQLITE",
            connectionName()
            );

        db.setDatabaseName(databasePath);
    }

    // Open database
    if (!db.open()) {
        if (errorMessage)
            *errorMessage = db.lastError().text();

        return false;
    }

    // Enable foreign keys
    {
        QSqlQuery foreignKeyQuery(db);

        if (!foreignKeyQuery.exec("PRAGMA foreign_keys = ON")) {
            if (errorMessage)
                *errorMessage = foreignKeyQuery.lastError().text();

            return false;
        }

        foreignKeyQuery.finish();
    }

    // ======================================
    // Create Database Tables
    // ======================================

    const QStringList statements = {

    // Students
    R"(
            CREATE TABLE IF NOT EXISTS students (
                student_id INTEGER PRIMARY KEY AUTOINCREMENT,
                university_id TEXT NOT NULL UNIQUE,
                full_name TEXT NOT NULL,

                roster_number INTEGER,
                student_status TEXT,
                pre_second_round_status TEXT,

                remaining_subjects INTEGER NOT NULL DEFAULT 0
                    CHECK (remaining_subjects >= 0),

                email TEXT,
                department TEXT,
                academic_level TEXT,

                created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
            )
        )",

        // Courses
        R"(
            CREATE TABLE IF NOT EXISTS courses (
                course_id INTEGER PRIMARY KEY AUTOINCREMENT,
                course_name TEXT NOT NULL UNIQUE,
                max_mark INTEGER NOT NULL
                    CHECK (max_mark > 0)
            )
        )",

        // Semesters
        R"(
            CREATE TABLE IF NOT EXISTS semesters (
                semester_id INTEGER PRIMARY KEY AUTOINCREMENT,
                semester_name TEXT NOT NULL,
                academic_year TEXT NOT NULL,
                UNIQUE (semester_name, academic_year)
            )
        )",

        // GPA grade scale
        R"(
            CREATE TABLE IF NOT EXISTS grade_scale (
                grade_scale_id INTEGER PRIMARY KEY AUTOINCREMENT,
                letter_grade TEXT NOT NULL UNIQUE,
                min_score REAL NOT NULL
                    CHECK (min_score BETWEEN 0 AND 100),
                max_score REAL NOT NULL
                    CHECK (max_score BETWEEN 0 AND 100),
                grade_points REAL NOT NULL
                    CHECK (grade_points BETWEEN 0 AND 4),
                CHECK (min_score <= max_score)
            )
        )",

        // Arabic grade types
        R"(
            CREATE TABLE IF NOT EXISTS grade_types (
                grade_code TEXT PRIMARY KEY,
                grade_name_ar TEXT NOT NULL
            )
        )",

        // Student course enrollment and results
        R"(
            CREATE TABLE IF NOT EXISTS enrollments (
                enrollment_id INTEGER PRIMARY KEY AUTOINCREMENT,

                student_id INTEGER NOT NULL,
                course_id INTEGER NOT NULL,
                semester_id INTEGER NOT NULL,

                final_score REAL
                    CHECK (
                        final_score IS NULL
                        OR final_score BETWEEN 0 AND 100
                    ),

                grade_scale_id INTEGER,
                grade_code TEXT,
                original_grade TEXT,

                grade_points REAL
                    CHECK (
                        grade_points IS NULL
                        OR grade_points BETWEEN 0 AND 4
                    ),

                FOREIGN KEY (student_id)
                    REFERENCES students(student_id)
                    ON DELETE CASCADE,

                FOREIGN KEY (course_id)
                    REFERENCES courses(course_id)
                    ON DELETE CASCADE,

                FOREIGN KEY (semester_id)
                    REFERENCES semesters(semester_id)
                    ON DELETE CASCADE,

                FOREIGN KEY (grade_scale_id)
                    REFERENCES grade_scale(grade_scale_id),

                FOREIGN KEY (grade_code)
                    REFERENCES grade_types(grade_code),

                UNIQUE (student_id, course_id, semester_id)
            )
        )",

        // Enrollment index
        R"(
            CREATE INDEX IF NOT EXISTS idx_enrollments_student_semester
            ON enrollments(student_id, semester_id)
        )"
};

// Execute creation statements
{
    QSqlQuery schemaQuery(db);

    for (const QString &statement : statements) {

        if (!schemaQuery.exec(statement)) {

            if (errorMessage) {
                *errorMessage =
                    "Database table creation failed:\n" +
                    schemaQuery.lastError().text();
            }

            schemaQuery.finish();
            return false;
        }

        schemaQuery.finish();
    }
}

// ======================================
// Courses Table Migration
// ======================================

bool hasMaxMark = false;

{
    QSqlQuery infoQuery(db);

    if (!infoQuery.exec("PRAGMA table_info(courses)")) {
        if (errorMessage)
            *errorMessage = infoQuery.lastError().text();

        return false;
    }

    while (infoQuery.next()) {

        if (infoQuery.value(1).toString() == "max_mark")
            hasMaxMark = true;
    }

    infoQuery.finish();
}

if (!hasMaxMark) {

    int courseCount = 0;
    int enrollmentCount = 0;

    // Check existing records
    {
        QSqlQuery countQuery(db);

        if (!countQuery.exec(R"(
                SELECT
                    (SELECT COUNT(*) FROM courses),
                    (SELECT COUNT(*) FROM enrollments)
            )") || !countQuery.next()) {

            if (errorMessage)
                *errorMessage = countQuery.lastError().text();

            countQuery.finish();
            return false;
        }

        courseCount = countQuery.value(0).toInt();
        enrollmentCount = countQuery.value(1).toInt();

        countQuery.finish();
    }

    // Protect existing records
    if (courseCount > 0 || enrollmentCount > 0) {

        if (errorMessage) {
            *errorMessage =
                "Courses migration stopped: existing records "
                "must be preserved before migration.";
        }

        return false;
    }

    // Disable foreign keys before transaction
    {
        QSqlQuery fkQuery(db);

        if (!fkQuery.exec("PRAGMA foreign_keys = OFF")) {
            if (errorMessage)
                *errorMessage = fkQuery.lastError().text();

            return false;
        }

        fkQuery.finish();
    }

    // Start transaction
    if (!db.transaction()) {

        if (errorMessage)
            *errorMessage = db.lastError().text();

        QSqlQuery fkQuery(db);
        fkQuery.exec("PRAGMA foreign_keys = ON");
        fkQuery.finish();

        return false;
    }

    bool migrationSucceeded = false;
    QString migrationError;

    {
        QSqlQuery migrationQuery(db);

        // Drop old empty table
        migrationSucceeded =
            migrationQuery.exec("DROP TABLE courses");

        if (!migrationSucceeded) {
            migrationError =
                migrationQuery.lastError().text();
        }

        migrationQuery.finish();

        // Create updated table
        if (migrationSucceeded) {

            migrationSucceeded = migrationQuery.exec(R"(
                    CREATE TABLE courses (
                        course_id INTEGER PRIMARY KEY AUTOINCREMENT,
                        course_name TEXT NOT NULL UNIQUE,
                        max_mark INTEGER NOT NULL
                            CHECK (max_mark > 0)
                    )
                )");

            if (!migrationSucceeded) {
                migrationError =
                    migrationQuery.lastError().text();
            }

            migrationQuery.finish();
        }
    }

    // Commit or rollback
    if (migrationSucceeded) {

        migrationSucceeded = db.commit();

        if (!migrationSucceeded) {
            migrationError = db.lastError().text();
            db.rollback();
        }

    }
    else {
        db.rollback();
    }

    // Re-enable foreign keys
    {
        QSqlQuery fkQuery(db);

        if (!fkQuery.exec("PRAGMA foreign_keys = ON")) {

            if (errorMessage)
                *errorMessage = fkQuery.lastError().text();

            return false;
        }

        fkQuery.finish();
    }

    if (!migrationSucceeded) {

        if (errorMessage) {
            *errorMessage =
                "Courses migration failed: " + migrationError;
        }

        return false;
    }
}

// ======================================
// Students Table Migration
// ======================================

QStringList existingStudentColumns;

{
    QSqlQuery infoQuery(db);

    if (!infoQuery.exec("PRAGMA table_info(students)")) {

        if (errorMessage)
            *errorMessage = infoQuery.lastError().text();

        return false;
    }

    while (infoQuery.next()) {
        existingStudentColumns.append(
            infoQuery.value(1).toString()
            );
    }

    infoQuery.finish();
}

const QStringList studentColumnNames = {
    "roster_number",
    "student_status",
    "pre_second_round_status",
    "remaining_subjects"
};

const QStringList studentColumnDefinitions = {
    "roster_number INTEGER",
    "student_status TEXT",
    "pre_second_round_status TEXT",
    "remaining_subjects INTEGER NOT NULL DEFAULT 0 "
    "CHECK (remaining_subjects >= 0)"
};

for (int i = 0; i < studentColumnNames.size(); ++i) {

    if (existingStudentColumns.contains(studentColumnNames[i]))
        continue;

    QSqlQuery migrationQuery(db);

    const QString sql =
        "ALTER TABLE students ADD COLUMN " +
        studentColumnDefinitions[i];

    if (!migrationQuery.exec(sql)) {

        if (errorMessage)
            *errorMessage = migrationQuery.lastError().text();

        migrationQuery.finish();
        return false;
    }

    migrationQuery.finish();
}

// ======================================
// Enrollments Table Migration
// ======================================

bool hasGradeCode = false;
bool hasOriginalGrade = false;

{
    QSqlQuery infoQuery(db);

    if (!infoQuery.exec("PRAGMA table_info(enrollments)")) {

        if (errorMessage)
            *errorMessage = infoQuery.lastError().text();

        return false;
    }

    while (infoQuery.next()) {

        const QString columnName =
            infoQuery.value(1).toString();

        if (columnName == "grade_code")
            hasGradeCode = true;

        if (columnName == "original_grade")
            hasOriginalGrade = true;
    }

    infoQuery.finish();
}

if (!hasGradeCode) {

    QSqlQuery migrationQuery(db);

    if (!migrationQuery.exec(
            "ALTER TABLE enrollments "
            "ADD COLUMN grade_code TEXT")) {

        if (errorMessage)
            *errorMessage = migrationQuery.lastError().text();

        return false;
    }

    migrationQuery.finish();
}

if (!hasOriginalGrade) {

    QSqlQuery migrationQuery(db);

    if (!migrationQuery.exec(
            "ALTER TABLE enrollments "
            "ADD COLUMN original_grade TEXT")) {

        if (errorMessage)
            *errorMessage = migrationQuery.lastError().text();

        return false;
    }

    migrationQuery.finish();
}

// ======================================
// Insert Arabic Grade Types
// ======================================

const QList<QPair<QString, QString>> gradeTypes = {

qMakePair(QStringLiteral("م"), QStringLiteral("ممتاز")),
    qMakePair(QStringLiteral("جـ جـ"), QStringLiteral("جيد جدًا")),
    qMakePair(QStringLiteral("جـ//"), QStringLiteral("جيد")),
    qMakePair(QStringLiteral("ل"), QStringLiteral("مقبول")),
    qMakePair(QStringLiteral("رل"), QStringLiteral("راسب لائحة")),
    qMakePair(QStringLiteral("ض"), QStringLiteral("ضعيف")),
    qMakePair(QStringLiteral("غ"), QStringLiteral("غياب"))
};

{
    QSqlQuery gradeQuery(db);

    gradeQuery.prepare(R"(
            INSERT OR IGNORE INTO grade_types
            (grade_code, grade_name_ar)
            VALUES (:code, :name)
        )");

    for (const auto &grade : gradeTypes) {

        gradeQuery.bindValue(":code", grade.first);
        gradeQuery.bindValue(":name", grade.second);

        if (!gradeQuery.exec()) {

            if (errorMessage)
                *errorMessage = gradeQuery.lastError().text();

            gradeQuery.finish();
            return false;
        }
    }

    gradeQuery.finish();
}

// Initialization completed
qInfo() << "Database initialized successfully:"
        << databasePath;

return true;
}

// ==========================================
// Add Student
// ==========================================

bool DatabaseManager::addStudent(
    const QString &universityId,
    const QString &fullName,
    const QString &email,
    const QString &department,
    const QString &academicLevel,
    QString *errorMessage
    )
{
    if (universityId.trimmed().isEmpty() ||
        fullName.trimmed().isEmpty()) {

        if (errorMessage)
            *errorMessage =
                "University ID and Full Name are required.";

        return false;
    }

    QSqlQuery query(database());

    query.prepare(R"(
        INSERT INTO students (
            university_id,
            full_name,
            email,
            department,
            academic_level
        )
        VALUES (
            :universityId,
            :fullName,
            :email,
            :department,
            :academicLevel
        )
    )");

    query.bindValue(":universityId", universityId.trimmed());
    query.bindValue(":fullName", fullName.trimmed());
    query.bindValue(":email", email.trimmed());
    query.bindValue(":department", department.trimmed());
    query.bindValue(":academicLevel", academicLevel.trimmed());

    if (!query.exec()) {

        if (errorMessage)
            *errorMessage = query.lastError().text();

        return false;
    }

    query.finish();
    return true;
}

// ==========================================
// Get All Students
// ==========================================

QSqlQuery DatabaseManager::getAllStudents(
    QString *errorMessage
    )
{
    QSqlQuery query(database());

    if (!query.exec(R"(
        SELECT
            student_id,
            university_id,
            full_name,
            email,
            department,
            academic_level
        FROM students
        ORDER BY full_name COLLATE NOCASE
    )")) {

        if (errorMessage)
            *errorMessage = query.lastError().text();
    }

    return query;
}

// ==========================================
// Update Student
// ==========================================

bool DatabaseManager::updateStudent(
    int studentId,
    const QString &universityId,
    const QString &fullName,
    const QString &email,
    const QString &department,
    const QString &academicLevel,
    QString *errorMessage
    )
{
    if (universityId.trimmed().isEmpty() ||
        fullName.trimmed().isEmpty()) {

        if (errorMessage)
            *errorMessage =
                "University ID and Full Name are required.";

        return false;
    }

    QSqlQuery query(database());

    query.prepare(R"(
        UPDATE students
        SET university_id = :universityId,
            full_name = :fullName,
            email = :email,
            department = :department,
            academic_level = :academicLevel
        WHERE student_id = :studentId
    )");

    query.bindValue(":universityId", universityId.trimmed());
    query.bindValue(":fullName", fullName.trimmed());
    query.bindValue(":email", email.trimmed());
    query.bindValue(":department", department.trimmed());
    query.bindValue(":academicLevel", academicLevel.trimmed());
    query.bindValue(":studentId", studentId);

    if (!query.exec()) {

        if (errorMessage)
            *errorMessage = query.lastError().text();

        return false;
    }

    const bool updated = query.numRowsAffected() > 0;

    query.finish();

    if (!updated) {

        if (errorMessage)
            *errorMessage = "Student was not found.";

        return false;
    }

    return true;
}

// ==========================================
// Delete Student
// ==========================================

bool DatabaseManager::deleteStudent(
    int studentId,
    QString *errorMessage
    )
{
    QSqlQuery query(database());

    query.prepare(R"(
        DELETE FROM students
        WHERE student_id = :studentId
    )");

    query.bindValue(":studentId", studentId);

    if (!query.exec()) {

        if (errorMessage)
            *errorMessage = query.lastError().text();

        return false;
    }

    const bool deleted = query.numRowsAffected() > 0;

    query.finish();

    if (!deleted) {

        if (errorMessage)
            *errorMessage = "Student was not found.";

        return false;
    }

    return true;
}
