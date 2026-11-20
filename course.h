
#pragma once

#include <QString>

class Course
{
private:
    QString courseName;
    int maxMark = 0;

public:
    // Constructors
    Course();

    Course(
        const QString &courseName,
        int maxMark
        );

    // Getters
    QString getCourseName() const;
    int getMaxMark() const;

    // Setters
    void setCourseName(const QString &name);
    bool setMaxMark(int mark);

    // Validation
    bool isValid(QString *errorMessage = nullptr) const;
};
