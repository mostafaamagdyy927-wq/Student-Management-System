
#pragma once

#include "student.h"
#include "course.h"
#include "gradetype.h"

#include <QString>

class StudentCourseResult
{
private:
    // Composition
    Student student;
    Course course;
    GradeType grade;

public:
    // Constructor
    StudentCourseResult(
        const Student &student,
        const Course &course,
        const GradeType &grade
        );

    // Getters
    const Student &getStudent() const;
    const Course &getCourse() const;
    const GradeType &getGrade() const;

    // Setter
    void setGrade(const GradeType &newGrade);

    // Validation
    bool isValid(
        QString *errorMessage = nullptr
        ) const;
};
