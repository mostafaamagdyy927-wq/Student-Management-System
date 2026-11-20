
#include "courserepository.h"
#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>

// ==========================================
// Add Course
// ==========================================

bool CourseRepository::addCourse(
    const Course &course,
    QString *errorMessage
    )
{
    if (!course.isValid(errorMessage))
        return false;

    QSqlQuery query(DatabaseManager::database());

    query.prepare(R"(
        INSERT INTO courses (
            course_name,
            max_mark
        )
        VALUES (
            :courseName,
            :maxMark
        )
    )");

    query.bindValue(
        ":courseName",
        course.getCourseName()
        );

    query.bindValue(
        ":maxMark",
        course.getMaxMark()
        );

    if (!query.exec()) {
        if (errorMessage)
            *errorMessage = query.lastError().text();

        return false;
    }

    query.finish();

    return true;
}

// ==========================================
// Get All Courses
// ==========================================

QList<Course> CourseRepository::getAllCourses(
    QString *errorMessage
    ) const
{
    QList<Course> courses;

    QSqlQuery query(DatabaseManager::database());

    if (!query.exec(R"(
        SELECT
            course_name,
            max_mark
        FROM courses
        ORDER BY course_name
    )")) {

        if (errorMessage)
            *errorMessage = query.lastError().text();

        return courses;
    }

    while (query.next()) {

        Course course(
            query.value("course_name").toString(),
            query.value("max_mark").toInt()
            );

        courses.append(course);
    }

    query.finish();

    return courses;
}
