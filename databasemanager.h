
#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class DatabaseManager final
{
public:

    // Database operations
    static bool initialize(
        QString *errorMessage = nullptr
        );

    static QSqlDatabase database();

    // Student operations
    static bool addStudent(
        const QString &universityId,
        const QString &fullName,
        const QString &email,
        const QString &department,
        const QString &academicLevel,
        QString *errorMessage = nullptr
        );

    static QSqlQuery getAllStudents(
        QString *errorMessage = nullptr
        );

    static bool updateStudent(
        int studentId,
        const QString &universityId,
        const QString &fullName,
        const QString &email,
        const QString &department,
        const QString &academicLevel,
        QString *errorMessage = nullptr
        );

    static bool deleteStudent(
        int studentId,
        QString *errorMessage = nullptr
        );

private:

    // Private connection management
    static QString connectionName();
};
