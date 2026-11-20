
#pragma once

#include "student.h"

#include <QList>
#include <QString>

class StudentRepository
{
public:
    // Save a student in the database
    bool addStudent(
        const Student &student,
        QString *errorMessage = nullptr
        );

    // Retrieve all students
    QList<Student> getAllStudents(
        QString *errorMessage = nullptr
        ) const;
};
