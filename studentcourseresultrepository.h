
#pragma once

#include "studentcourseresult.h"

#include <QList>
#include <QString>

class StudentCourseResultRepository
{
public:

    // Save a student's annual course result
    bool addResult(
        const StudentCourseResult &result,
        const QString &academicYear,
        QString *errorMessage = nullptr
        );

    // Retrieve all annual results for a student
    QList<StudentCourseResult> getResultsForStudent(
        const QString &universityId,
        const QString &academicYear,
        QString *errorMessage = nullptr
        ) const;
};
