
#pragma once

#include <QString>

class Student
{
private:
    // Private attributes (Encapsulation)
    int rosterNumber = 0;
    QString universityId;
    QString fullName;
    QString status;
    QString preSecondRoundStatus;
    int remainingSubjects = 0;

public:
    // Constructors
    Student();

    // Existing constructor
    Student(
        const QString &universityId,
        const QString &fullName,
        const QString &status,
        const QString &preSecondRoundStatus,
        int remainingSubjects
        );

    // Constructor with roster number
    Student(
        int rosterNumber,
        const QString &universityId,
        const QString &fullName,
        const QString &status,
        const QString &preSecondRoundStatus,
        int remainingSubjects
        );

    // Getters
    int getRosterNumber() const;
    QString getUniversityId() const;
    QString getFullName() const;
    QString getStatus() const;
    QString getPreSecondRoundStatus() const;
    int getRemainingSubjects() const;

    // Setters
    bool setRosterNumber(int number);
    void setUniversityId(const QString &id);
    void setFullName(const QString &name);
    void setStatus(const QString &studentStatus);
    void setPreSecondRoundStatus(const QString &academicStatus);
    bool setRemainingSubjects(int count);

    // Validation
    bool isValid(QString *errorMessage = nullptr) const;
};
