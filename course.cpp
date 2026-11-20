
#include "course.h"

// Default Constructor
Course::Course()
{
}

// Parameterized Constructor
Course::Course(
    const QString &courseName,
    int maxMark
    )
{
    setCourseName(courseName);
    setMaxMark(maxMark);
}

// Getters

QString Course::getCourseName() const
{
    return courseName;
}

int Course::getMaxMark() const
{
    return maxMark;
}

// Setters

void Course::setCourseName(const QString &name)
{
    courseName = name.trimmed();
}

bool Course::setMaxMark(int mark)
{
    if (mark <= 0)
        return false;

    maxMark = mark;
    return true;
}

// Validation

bool Course::isValid(QString *errorMessage) const
{
    if (courseName.isEmpty()) {
        if (errorMessage)
            *errorMessage = "Course name is required.";

        return false;
    }

    if (maxMark <= 0) {
        if (errorMessage)
            *errorMessage = "Maximum mark must be positive.";

        return false;
    }

    return true;
}
