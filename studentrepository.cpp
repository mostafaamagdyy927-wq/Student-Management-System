
#include "studentrepository.h"
#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

// ==========================================
// Add Student
// ==========================================

bool StudentRepository::addStudent(
    const Student &student,
    QString *errorMessage
    )
{
    // Validate Student object
    if (!student.isValid(errorMessage))
        return false;

    QSqlQuery query(DatabaseManager::database());

    query.prepare(R"(
        INSERT INTO students (
            roster_number,
            university_id,
            full_name,
            student_status,
            pre_second_round_status,
            remaining_subjects
        )
        VALUES (
            :rosterNumber,
            :universityId,
            :fullName,
            :studentStatus,
            :preSecondRoundStatus,
            :remainingSubjects
        )
    )");

    query.bindValue(
        ":rosterNumber",
        student.getRosterNumber()
        );

    query.bindValue(
        ":universityId",
        student.getUniversityId()
        );

    query.bindValue(
        ":fullName",
        student.getFullName()
        );

    query.bindValue(
        ":studentStatus",
        student.getStatus()
        );

    query.bindValue(
        ":preSecondRoundStatus",
        student.getPreSecondRoundStatus()
        );

    query.bindValue(
        ":remainingSubjects",
        student.getRemainingSubjects()
        );

    if (!query.exec()) {
        if (errorMessage)
            *errorMessage = query.lastError().text();

        return false;
    }

    return true;
}

// ==========================================
// Get All Students
// ==========================================

QList<Student> StudentRepository::getAllStudents(
    QString *errorMessage
    ) const
{
    QList<Student> students;

    QSqlQuery query(DatabaseManager::database());

    if (!query.exec(R"(
        SELECT
            roster_number,
            university_id,
            full_name,
            student_status,
            pre_second_round_status,
            remaining_subjects
        FROM students
        ORDER BY roster_number
    )")) {

        if (errorMessage)
            *errorMessage = query.lastError().text();

        return students;
    }

    while (query.next()) {

        Student student(
            query.value("roster_number").toInt(),
            query.value("university_id").toString(),
            query.value("full_name").toString(),
            query.value("student_status").toString(),
            query.value("pre_second_round_status").toString(),
            query.value("remaining_subjects").toInt()
            );

        students.append(student);
    }

    return students;
}
