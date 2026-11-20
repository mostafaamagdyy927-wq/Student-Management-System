
#include "student.h"

// ==========================================
// Constructors
// ==========================================

Student::Student()
{
}

// Existing constructor
Student::Student(
    const QString &universityId,
    const QString &fullName,
    const QString &status,
    const QString &preSecondRoundStatus,
    int remainingSubjects
    )
    : Student(
          0,
          universityId,
          fullName,
          status,
          preSecondRoundStatus,
          remainingSubjects
          )
{
}

// Constructor with roster number
Student::Student(
    int rosterNumber,
    const QString &universityId,
    const QString &fullName,
    const QString &status,
    const QString &preSecondRoundStatus,
    int remainingSubjects
    )
{
    setRosterNumber(rosterNumber);
    setUniversityId(universityId);
    setFullName(fullName);
    setStatus(status);
    setPreSecondRoundStatus(preSecondRoundStatus);
    setRemainingSubjects(remainingSubjects);
}

// ==========================================
// Getters
// ==========================================

int Student::getRosterNumber() const
{
    return rosterNumber;
}

QString Student::getUniversityId() const
{
    return universityId;
}

QString Student::getFullName() const
{
    return fullName;
}

QString Student::getStatus() const
{
    return status;
}

QString Student::getPreSecondRoundStatus() const
{
    return preSecondRoundStatus;
}

int Student::getRemainingSubjects() const
{
    return remainingSubjects;
}

// ==========================================
// Setters
// ==========================================

bool Student::setRosterNumber(int number)
{
    if (number < 0)
        return false;

    rosterNumber = number;
    return true;
}

void Student::setUniversityId(const QString &id)
{
    universityId = id.trimmed();
}

void Student::setFullName(const QString &name)
{
    fullName = name.trimmed();
}

void Student::setStatus(const QString &studentStatus)
{
    status = studentStatus.trimmed();
}

void Student::setPreSecondRoundStatus(
    const QString &academicStatus
    )
{
    preSecondRoundStatus = academicStatus.trimmed();
}

bool Student::setRemainingSubjects(int count)
{
    if (count < 0)
        return false;

    remainingSubjects = count;
    return true;
}

// ==========================================
// Validation
// ==========================================

bool Student::isValid(QString *errorMessage) const
{
    if (universityId.isEmpty()) {
        if (errorMessage)
            *errorMessage = "University ID is required.";

        return false;
    }

    if (fullName.isEmpty()) {
        if (errorMessage)
            *errorMessage = "Student name is required.";

        return false;
    }

    if (rosterNumber < 0 || remainingSubjects < 0) {
        if (errorMessage)
            *errorMessage = "Invalid student data.";

        return false;
    }

    return true;
}
