
#pragma once

#include "course.h"

#include <QList>
#include <QString>

class CourseRepository
{
public:
    bool addCourse(
        const Course &course,
        QString *errorMessage = nullptr
        );

    QList<Course> getAllCourses(
        QString *errorMessage = nullptr
        ) const;
};
