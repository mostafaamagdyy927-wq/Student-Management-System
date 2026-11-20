#include "dataimporter.h"
#include "databasemanager.h"

#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QRegularExpression>
#include <QSet>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace
{
struct StudentCsvRow
{
    int rosterNumber = 0;
    QString universityId;
    QString fullName;
    QString studentStatus;
    QString preSecondRoundStatus;
    int remainingSubjects = 0;
    QStringList grades;
};

struct CourseCsvInfo
{
    QString name;
    int maxMark = 0;
};

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage)
        *errorMessage = message;
}

// Normalize known source variants to the canonical grade code.
// The original CSV token is stored separately in original_grade.
QString normalizeGradeCode(const QString &rawGrade)
{
    const QString value = rawGrade.trimmed();

    // The source uses both "ل" and "(ل)" for the same grade.
    if (value == QStringLiteral("(ل)"))
        return QStringLiteral("ل");

    return value;
}
}

// ==========================================
// CSV Parser
// Supports commas inside quoted fields and escaped quotes.
// This CSV is UTF-8 and has one record per line.
// ==========================================

QStringList DataImporter::parseCsvLine(const QString &line)
{
    QStringList fields;
    QString field;
    bool insideQuotes = false;

    for (qsizetype i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);

        if (ch == QLatin1Char('"')) {
            if (insideQuotes && i + 1 < line.size()
                && line.at(i + 1) == QLatin1Char('"')) {
                field += QLatin1Char('"');
                ++i;
            } else {
                insideQuotes = !insideQuotes;
            }
        } else if (ch == QLatin1Char(',') && !insideQuotes) {
            fields.append(field);
            field.clear();
        } else {
            field += ch;
        }
    }

    fields.append(field);
    return fields;
}

// ==========================================
// Import CSV into SQLite
// Re-importing the same file updates existing rows
// instead of creating duplicates.
// ==========================================

bool DataImporter::importCsv(
    const QString &filePath,
    const QString &academicYear,
    ImportSummary *summary,
    QString *errorMessage
) const
{
    if (summary)
        *summary = ImportSummary{};

    if (filePath.trimmed().isEmpty()) {
        setError(errorMessage, "CSV file path is empty.");
        return false;
    }

    if (academicYear.trimmed().isEmpty()) {
        setError(errorMessage, "Academic year is required.");
        return false;
    }

    // --------------------------------------
    // 1. Read and validate CSV before DB writes
    // --------------------------------------

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(errorMessage, "Could not open CSV file: " + file.errorString());
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    if (stream.atEnd()) {
        setError(errorMessage, "CSV file is empty.");
        return false;
    }

    QString headerLine = stream.readLine();
    if (headerLine.startsWith(QChar(0xFEFF)))
        headerLine.remove(0, 1); // Remove UTF-8 BOM if present.

    const QStringList headers = parseCsvLine(headerLine);

    constexpr int expectedColumnCount = 22;
    constexpr int firstCourseColumn = 4;
    constexpr int courseCount = 16;

    if (headers.size() != expectedColumnCount) {
        setError(
            errorMessage,
            QString("Invalid CSV header: expected %1 columns, found %2.")
                .arg(expectedColumnCount)
                .arg(headers.size())
        );
        return false;
    }

    if (headers.at(0).trimmed() != QStringLiteral("مسلسل")
        || headers.at(1).trimmed() != QStringLiteral("رقم الكود")
        || headers.at(2).trimmed() != QStringLiteral("اسم الطالب")
        || headers.at(3).trimmed() != QStringLiteral("الحالة")
        || headers.at(20).trimmed() != QStringLiteral("حالة الطالب قبل الدور الثاني")
        || headers.at(21).trimmed() != QStringLiteral("حالة الطالب (عدد المواد)")) {
        setError(errorMessage, "CSV columns do not match the expected student-results format.");
        return false;
    }

    QList<CourseCsvInfo> courses;
    const QRegularExpression courseHeaderPattern(
        QStringLiteral(R"(^\s*(.*?)\s*\((\d+)\)\s*$)")
    );

    for (int column = firstCourseColumn;
         column < firstCourseColumn + courseCount;
         ++column) {

        const auto match = courseHeaderPattern.match(headers.at(column));
        if (!match.hasMatch()) {
            setError(
                errorMessage,
                "Invalid course column header: " + headers.at(column)
            );
            return false;
        }

        CourseCsvInfo course;
        course.name = match.captured(1).trimmed();
        bool markOk = false;
        course.maxMark = match.captured(2).toInt(&markOk);

        if (course.name.isEmpty() || !markOk || course.maxMark <= 0) {
            setError(
                errorMessage,
                "Invalid course name or maximum mark in header: " + headers.at(column)
            );
            return false;
        }

        for (const CourseCsvInfo &existing : courses) {
            if (existing.name.compare(course.name, Qt::CaseInsensitive) == 0) {
                setError(errorMessage, "Duplicate course name in CSV: " + course.name);
                return false;
            }
        }

        courses.append(course);
    }

    QList<StudentCsvRow> students;
    QSet<QString> seenUniversityIds;
    QSet<int> seenRosterNumbers;
    int csvLineNumber = 1;

    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        ++csvLineNumber;

        if (line.trimmed().isEmpty())
            continue;

        const QStringList fields = parseCsvLine(line);
        if (fields.size() != expectedColumnCount) {
            setError(
                errorMessage,
                QString("CSV line %1 has %2 columns; expected %3.")
                    .arg(csvLineNumber)
                    .arg(fields.size())
                    .arg(expectedColumnCount)
            );
            return false;
        }

        StudentCsvRow student;

        bool rosterOk = false;
        student.rosterNumber = fields.at(0).trimmed().toInt(&rosterOk);
        student.universityId = fields.at(1).trimmed();
        student.fullName = fields.at(2).trimmed();
        student.studentStatus = fields.at(3).trimmed();
        student.preSecondRoundStatus = fields.at(20).trimmed();

        bool remainingOk = false;
        student.remainingSubjects = fields.at(21).trimmed().toInt(&remainingOk);

        if (!rosterOk || student.rosterNumber <= 0) {
            setError(errorMessage, QString("Invalid serial number on CSV line %1.").arg(csvLineNumber));
            return false;
        }

        if (student.universityId.isEmpty() || student.fullName.isEmpty()) {
            setError(errorMessage, QString("Missing student ID or name on CSV line %1.").arg(csvLineNumber));
            return false;
        }

        if (!remainingOk || student.remainingSubjects < 0) {
            setError(errorMessage, QString("Invalid remaining-subject count on CSV line %1.").arg(csvLineNumber));
            return false;
        }

        if (seenUniversityIds.contains(student.universityId)) {
            setError(errorMessage, "Duplicate university ID in CSV: " + student.universityId);
            return false;
        }

        if (seenRosterNumbers.contains(student.rosterNumber)) {
            setError(errorMessage, QString("Duplicate roster number in CSV: %1").arg(student.rosterNumber));
            return false;
        }

        seenUniversityIds.insert(student.universityId);
        seenRosterNumbers.insert(student.rosterNumber);

        for (int column = firstCourseColumn;
             column < firstCourseColumn + courseCount;
             ++column) {
            student.grades.append(fields.at(column));
        }

        students.append(student);
    }

    file.close();

    if (students.isEmpty()) {
        setError(errorMessage, "No student records were found in the CSV file.");
        return false;
    }

    // --------------------------------------
    // 2. Prepare grade-code recognition
    // --------------------------------------

    const QSet<QString> knownGradeCodes = {
        QStringLiteral("م"),
        QStringLiteral("جـ جـ"),
        QStringLiteral("جـ//"),
        QStringLiteral("ل"),
        QStringLiteral("رل"),
        QStringLiteral("ض"),
        QStringLiteral("غ")
    };

    // --------------------------------------
    // 3. Start one transaction for the import
    // --------------------------------------

    QSqlDatabase db = DatabaseManager::database();
    if (!db.isValid() || !db.isOpen()) {
        setError(errorMessage, "Database is not open.");
        return false;
    }

    if (!db.transaction()) {
        setError(errorMessage, "Could not start import transaction: " + db.lastError().text());
        return false;
    }

    auto failImport = [&](const QString &message) -> bool {
        db.rollback();
        setError(errorMessage, message);
        return false;
    };

    // --------------------------------------
    // 4. Create or update the full-year record
    // --------------------------------------

    const QString semesterName = QStringLiteral("Full Year");

    {
        QSqlQuery semesterInsert(db);
        semesterInsert.prepare(R"(
            INSERT OR IGNORE INTO semesters (semester_name, academic_year)
            VALUES (:semesterName, :academicYear)
        )");
        semesterInsert.bindValue(":semesterName", semesterName);
        semesterInsert.bindValue(":academicYear", academicYear.trimmed());

        if (!semesterInsert.exec()) {
            const QString error = semesterInsert.lastError().text();
            semesterInsert.finish();
            return failImport("Could not create academic year: " + error);
        }
        semesterInsert.finish();
    }

    int semesterId = -1;
    {
        QSqlQuery semesterSelect(db);
        semesterSelect.prepare(R"(
            SELECT semester_id
            FROM semesters
            WHERE semester_name = :semesterName
              AND academic_year = :academicYear
        )");
        semesterSelect.bindValue(":semesterName", semesterName);
        semesterSelect.bindValue(":academicYear", academicYear.trimmed());

        if (!semesterSelect.exec() || !semesterSelect.next()) {
            const QString error = semesterSelect.lastError().text();
            semesterSelect.finish();
            return failImport("Could not find academic year: " + error);
        }

        semesterId = semesterSelect.value(0).toInt();
        semesterSelect.finish();
    }

    // --------------------------------------
    // 5. Insert/update courses
    // --------------------------------------

    QHash<QString, int> courseIds;

    for (const CourseCsvInfo &course : courses) {
        {
            QSqlQuery insertCourse(db);
            insertCourse.prepare(R"(
                INSERT INTO courses (course_name, max_mark)
                VALUES (:courseName, :maxMark)
                ON CONFLICT(course_name)
                DO UPDATE SET max_mark = excluded.max_mark
            )");
            insertCourse.bindValue(":courseName", course.name);
            insertCourse.bindValue(":maxMark", course.maxMark);

            if (!insertCourse.exec()) {
                const QString error = insertCourse.lastError().text();
                insertCourse.finish();
                return failImport("Could not save course '" + course.name + "': " + error);
            }
            insertCourse.finish();
        }

        {
            QSqlQuery selectCourse(db);
            selectCourse.prepare(
                "SELECT course_id FROM courses WHERE course_name = :courseName"
            );
            selectCourse.bindValue(":courseName", course.name);

            if (!selectCourse.exec() || !selectCourse.next()) {
                const QString error = selectCourse.lastError().text();
                selectCourse.finish();
                return failImport("Could not retrieve course '" + course.name + "': " + error);
            }

            courseIds.insert(course.name, selectCourse.value(0).toInt());
            selectCourse.finish();
        }
    }

    // --------------------------------------
    // 6. Prepare reusable student queries
    // --------------------------------------

    QHash<QString, int> studentIds;

    for (const StudentCsvRow &student : students) {
        {
            QSqlQuery insertStudent(db);
            insertStudent.prepare(R"(
                INSERT INTO students (
                    roster_number,
                    university_id,
                    full_name,
                    student_status,
                    pre_second_round_status,
                    remaining_subjects
                )
                VALUES (
                    :rosterNumber,
                    :universityId,
                    :fullName,
                    :studentStatus,
                    :preSecondRoundStatus,
                    :remainingSubjects
                )
                ON CONFLICT(university_id)
                DO UPDATE SET
                    roster_number = excluded.roster_number,
                    full_name = excluded.full_name,
                    student_status = excluded.student_status,
                    pre_second_round_status = excluded.pre_second_round_status,
                    remaining_subjects = excluded.remaining_subjects
            )");

            insertStudent.bindValue(":rosterNumber", student.rosterNumber);
            insertStudent.bindValue(":universityId", student.universityId);
            insertStudent.bindValue(":fullName", student.fullName);
            insertStudent.bindValue(":studentStatus", student.studentStatus);
            insertStudent.bindValue(":preSecondRoundStatus", student.preSecondRoundStatus);
            insertStudent.bindValue(":remainingSubjects", student.remainingSubjects);

            if (!insertStudent.exec()) {
                const QString error = insertStudent.lastError().text();
                insertStudent.finish();
                return failImport("Could not save student '" + student.universityId + "': " + error);
            }
            insertStudent.finish();
        }

        {
            QSqlQuery selectStudent(db);
            selectStudent.prepare(
                "SELECT student_id FROM students WHERE university_id = :universityId"
            );
            selectStudent.bindValue(":universityId", student.universityId);

            if (!selectStudent.exec() || !selectStudent.next()) {
                const QString error = selectStudent.lastError().text();
                selectStudent.finish();
                return failImport("Could not retrieve student '" + student.universityId + "': " + error);
            }

            studentIds.insert(student.universityId, selectStudent.value(0).toInt());
            selectStudent.finish();
        }
    }

    // --------------------------------------
    // 7. Insert/update all 16 results per student
    // --------------------------------------

    int resultsProcessed = 0;
    int blankGrades = 0;
    int unrecognizedGrades = 0;

    QSqlQuery resultQuery(db);
    resultQuery.prepare(R"(
        INSERT INTO enrollments (
            student_id,
            course_id,
            semester_id,
            final_score,
            grade_scale_id,
            grade_code,
            original_grade,
            grade_points
        )
        VALUES (
            :studentId,
            :courseId,
            :semesterId,
            NULL,
            NULL,
            :gradeCode,
            :originalGrade,
            NULL
        )
        ON CONFLICT(student_id, course_id, semester_id)
        DO UPDATE SET
            final_score = NULL,
            grade_scale_id = NULL,
            grade_code = excluded.grade_code,
            original_grade = excluded.original_grade,
            grade_points = NULL
    )");

    for (const StudentCsvRow &student : students) {
        const int studentId = studentIds.value(student.universityId, -1);
        if (studentId < 0) {
            resultQuery.finish();
            return failImport("Missing database ID for student " + student.universityId);
        }

        for (int courseIndex = 0; courseIndex < courses.size(); ++courseIndex) {
            const QString rawGrade = student.grades.at(courseIndex);
            const QString normalizedGrade = normalizeGradeCode(rawGrade);

            QVariant gradeCodeValue;
            QVariant originalGradeValue;

            if (normalizedGrade.isEmpty()) {
                ++blankGrades;
            } else {
                // Preserve the exact source token, including parentheses.
                originalGradeValue = rawGrade;

                if (knownGradeCodes.contains(normalizedGrade)) {
                    // Store the canonical code used by the application.
                    // For example, source "(ل)" is stored as grade_code "ل".
                    gradeCodeValue = normalizedGrade;
                } else {
                    // Preserve unknown source tokens without inventing a grade.
                    // grade_code remains NULL because it is not in grade_types.
                    ++unrecognizedGrades;
                }
            }

            resultQuery.bindValue(":studentId", studentId);
            resultQuery.bindValue(":courseId", courseIds.value(courses.at(courseIndex).name));
            resultQuery.bindValue(":semesterId", semesterId);
            resultQuery.bindValue(":gradeCode", gradeCodeValue);
            resultQuery.bindValue(":originalGrade", originalGradeValue);

            if (!resultQuery.exec()) {
                const QString error = resultQuery.lastError().text();
                resultQuery.finish();
                return failImport(
                    QString("Could not save result for student %1, course %2: %3")
                        .arg(student.universityId)
                        .arg(courses.at(courseIndex).name, error)
                );
            }

            resultQuery.finish();
            ++resultsProcessed;
        }
    }

    // --------------------------------------
    // 8. Commit all imported data
    // --------------------------------------

    if (!db.commit()) {
        const QString error = db.lastError().text();
        db.rollback();
        setError(errorMessage, "Could not commit CSV import: " + error);
        return false;
    }

    if (summary) {
        summary->studentsProcessed = students.size();
        summary->coursesProcessed = courses.size();
        summary->resultsProcessed = resultsProcessed;
        summary->blankGrades = blankGrades;
        summary->unrecognizedGrades = unrecognizedGrades;
    }

    return true;
}
