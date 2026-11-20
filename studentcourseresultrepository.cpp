
#include "studentcourseresultrepository.h"
#include "databasemanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

// ==========================================
// Add Student Course Result
// ==========================================

bool StudentCourseResultRepository::addResult(
    const StudentCourseResult &result,
    const QString &academicYear,
    QString *errorMessage
    )
{
    // Validate input
    if (!result.isValid(errorMessage))
        return false;

    if (academicYear.trimmed().isEmpty()) {
        if (errorMessage)
            *errorMessage = "Academic year is required.";

        return false;
    }

    QSqlDatabase db = DatabaseManager::database();

    if (!db.isOpen()) {
        if (errorMessage)
            *errorMessage = "Database is not open.";

        return false;
    }

    // Start transaction
    if (!db.transaction()) {
        if (errorMessage)
            *errorMessage = db.lastError().text();

        return false;
    }

    auto fail = [&](const QString &message) {
        if (errorMessage)
            *errorMessage = message;

        db.rollback();
        return false;
    };

    const Student student = result.getStudent();
    const Course course = result.getCourse();
    const GradeType grade = result.getGrade();

    // ======================================
    // Find Student
    // ======================================

    int studentId = -1;

    {
        QSqlQuery query(db);

        query.prepare(R"(
            SELECT student_id
            FROM students
            WHERE university_id = :universityId
        )");

        query.bindValue(
            ":universityId",
            student.getUniversityId()
            );

        if (!query.exec())
            return fail(query.lastError().text());

        if (!query.next())
            return fail("Student was not found in database.");

        studentId = query.value(0).toInt();
        query.finish();
    }

    // ======================================
    // Find Course
    // ======================================

    int courseId = -1;

    {
        QSqlQuery query(db);

        query.prepare(R"(
            SELECT course_id
            FROM courses
            WHERE course_name = :courseName
        )");

        query.bindValue(
            ":courseName",
            course.getCourseName()
            );

        if (!query.exec())
            return fail(query.lastError().text());

        if (!query.next())
            return fail("Course was not found in database.");

        courseId = query.value(0).toInt();
        query.finish();
    }

    // ======================================
    // Find or Create Full Year Semester
    // ======================================

    const QString semesterName = "Full Year";

    {
        QSqlQuery insertSemester(db);

        insertSemester.prepare(R"(
            INSERT OR IGNORE INTO semesters (
                semester_name,
                academic_year
            )
            VALUES (
                :semesterName,
                :academicYear
            )
        )");

        insertSemester.bindValue(
            ":semesterName",
            semesterName
            );

        insertSemester.bindValue(
            ":academicYear",
            academicYear.trimmed()
            );

        if (!insertSemester.exec())
            return fail(insertSemester.lastError().text());

        insertSemester.finish();
    }

    int semesterId = -1;

    {
        QSqlQuery query(db);

        query.prepare(R"(
            SELECT semester_id
            FROM semesters
            WHERE semester_name = :semesterName
              AND academic_year = :academicYear
        )");

        query.bindValue(":semesterName", semesterName);
        query.bindValue(":academicYear", academicYear.trimmed());

        if (!query.exec())
            return fail(query.lastError().text());

        if (!query.next())
            return fail("Could not find the academic year.");

        semesterId = query.value(0).toInt();
        query.finish();
    }

    // ======================================
    // Save Grade
    // ======================================

    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO enrollments (
            student_id,
            course_id,
            semester_id,
            final_score,
            grade_scale_id,
            grade_code,
            original_grade,
            grade_points
        )
        VALUES (
            :studentId,
            :courseId,
            :semesterId,
            NULL,
            NULL,
            :gradeCode,
            :originalGrade,
            NULL
        )
    )");

    query.bindValue(":studentId", studentId);
    query.bindValue(":courseId", courseId);
    query.bindValue(":semesterId", semesterId);

    // Empty grades remain SQL NULL
    const QVariant gradeValue = grade.isEmpty()
                                    ? QVariant()
                                    : QVariant(grade.getCode());

    query.bindValue(":gradeCode", gradeValue);
    query.bindValue(":originalGrade", gradeValue);

    if (!query.exec())
        return fail(query.lastError().text());

    query.finish();

    // Commit transaction
    if (!db.commit()) {
        const QString error = db.lastError().text();
        db.rollback();

        if (errorMessage)
            *errorMessage = error;

        return false;
    }

    return true;
}

// ==========================================
// Get Results For Student
// ==========================================

QList<StudentCourseResult>
StudentCourseResultRepository::getResultsForStudent(
    const QString &universityId,
    const QString &academicYear,
    QString *errorMessage
    ) const
{
    QList<StudentCourseResult> results;

    QSqlQuery query(DatabaseManager::database());

    query.prepare(R"(
        SELECT
            s.roster_number,
            s.university_id,
            s.full_name,
            s.student_status,
            s.pre_second_round_status,
            s.remaining_subjects,

            c.course_name,
            c.max_mark,

            e.grade_code

        FROM enrollments e

        INNER JOIN students s
            ON e.student_id = s.student_id

        INNER JOIN courses c
            ON e.course_id = c.course_id

        INNER JOIN semesters sem
            ON e.semester_id = sem.semester_id

        WHERE s.university_id = :universityId
          AND sem.academic_year = :academicYear

        ORDER BY c.course_name
    )");

    query.bindValue(":universityId", universityId);
    query.bindValue(":academicYear", academicYear);

    if (!query.exec()) {
        if (errorMessage)
            *errorMessage = query.lastError().text();

        return results;
    }

    while (query.next()) {

        // Rebuild Student Object
        Student student(
            query.value("roster_number").toInt(),
            query.value("university_id").toString(),
            query.value("full_name").toString(),
            query.value("student_status").toString(),
            query.value("pre_second_round_status").toString(),
            query.value("remaining_subjects").toInt()
            );

        // Rebuild Course Object
        Course course(
            query.value("course_name").toString(),
            query.value("max_mark").toInt()
            );

        // Rebuild GradeType Object
        const QString gradeCode =
            query.value("grade_code").toString();

        GradeType grade = gradeCode.isEmpty()
                              ? GradeType()
                              : GradeType::fromCode(gradeCode);

        // Rebuild Result Object
        StudentCourseResult result(
            student,
            course,
            grade
            );

        results.append(result);
    }

    if (query.lastError().isValid() && errorMessage)
        *errorMessage = query.lastError().text();

    query.finish();

    return results;
}
#include "studentcourseresultrepository.h"

