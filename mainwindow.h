#pragma once

#include <QMainWindow>
#include <QList>
#include "student.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLabel;
class QLineEdit;
class QStackedWidget;
class QTableWidget;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupInterface();

    QWidget *createDashboardPage();
    QWidget *createStudentsPage();
    QWidget *createAcademicRecordPage();
    QWidget *createReportsPage();

    void loadDashboardStats();
    void loadStudents();
    void filterStudents(const QString &text);
    void openSelectedStudentRecord();
    void showAcademicRecord(const QString &universityId);
    void loadReports();

    Ui::MainWindow *ui;
    QStackedWidget *pages;

    // Students page
    QTableWidget *studentsTable;
    QLineEdit *studentSearch;
    QLabel *studentsCountLabel;
    QPushButton *viewRecordButton;

    // Dashboard statistics
    QLabel *dashboardStudentsLabel;
    QLabel *dashboardCoursesLabel;
    QLabel *dashboardResultsLabel;
    QLabel *dashboardPendingStudentsLabel;

    // Academic record page
    QLabel *recordStudentNameLabel;
    QLabel *recordStudentInfoLabel;
    QLabel *recordCountLabel;
    QTableWidget *recordsTable;

    // Reports page
    QLabel *reportTotalResultsLabel;
    QLabel *reportRecognizedGradesLabel;
    QLabel *reportBlankGradesLabel;
    QLabel *reportUnknownGradesLabel;
    QTableWidget *gradeDistributionTable;
    QTableWidget *studentStatusTable;
    QTableWidget *courseCoverageTable;

    QList<Student> studentsCache;
};
