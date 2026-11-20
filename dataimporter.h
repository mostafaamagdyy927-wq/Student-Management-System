#pragma once

#include <QString>
#include <QStringList>

class DataImporter
{
public:
    struct ImportSummary
    {
        int studentsProcessed = 0;
        int coursesProcessed = 0;
        int resultsProcessed = 0;
        int blankGrades = 0;
        int unrecognizedGrades = 0;
    };

    bool importCsv(
        const QString &filePath,
        const QString &academicYear,
        ImportSummary *summary = nullptr,
        QString *errorMessage = nullptr
    ) const;

private:
    static QStringList parseCsvLine(const QString &line);
};
