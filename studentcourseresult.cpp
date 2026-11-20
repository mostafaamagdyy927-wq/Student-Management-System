
#include "studentcourseresult.h"

// ==========================================
// Constructor
// ==========================================

StudentCourseResult::StudentCourseResult(
    const Student &student,
    const Course &course,
    const GradeType &grade
    )
    : student(student),
    course(course),
    grade(grade)
{
}

// ==========================================
// Getters
// ==========================================

const Student &StudentCourseResult::getStudent() const
{
    return student;
}

const Course &StudentCourseResult::getCourse() const
{
    return course;
}

const GradeType &StudentCourseResult::getGrade() const
{
    return grade;
}

// ==========================================
// Setter
// ==========================================

void StudentCourseResult::setGrade(
    const GradeType &newGrade
    )
{
    grade = newGrade;
}

// ==========================================
// Validation
// ==========================================

bool StudentCourseResult::isValid(
    QString *errorMessage
    ) const
{
    // Validate student
    if (!student.isValid(errorMessage))
        return false;

    // Validate course
    if (!course.isValid(errorMessage))
        return false;

    // Grade is optional
    // But if provided, it must be valid
    if (!grade.isEmpty() && !grade.isValid()) {

        if (errorMessage)
            *errorMessage = "Invalid grade type.";

        return false;
    }

    return true;
}
