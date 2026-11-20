#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "databasemanager.h"
#include "studentrepository.h"
#include "studentcourseresultrepository.h"
#include "studentcourseresult.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QFont>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSqlError>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStringList>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>

namespace
{
constexpr auto kAcademicYear = "2025/2026";

void prepareReadOnlyTable(QTableWidget *table)
{
    table->setLayoutDirection(Qt::RightToLeft);
    table->setAlternatingRowColors(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
}

void setTableText(QTableWidget *table, int row, int column,
                  const QString &text, bool centered = false)
{
    auto *item = new QTableWidgetItem(text);
    if (centered)
        item->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, column, item);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    pages(nullptr),
    studentsTable(nullptr),
    studentSearch(nullptr),
    studentsCountLabel(nullptr),
    viewRecordButton(nullptr),
    dashboardStudentsLabel(nullptr),
    dashboardCoursesLabel(nullptr),
    dashboardResultsLabel(nullptr),
    dashboardPendingStudentsLabel(nullptr),
    recordStudentNameLabel(nullptr),
    recordStudentInfoLabel(nullptr),
    recordCountLabel(nullptr),
    recordsTable(nullptr),
    reportTotalResultsLabel(nullptr),
    reportRecognizedGradesLabel(nullptr),
    reportBlankGradesLabel(nullptr),
    reportUnknownGradesLabel(nullptr),
    gradeDistributionTable(nullptr),
    studentStatusTable(nullptr),
    courseCoverageTable(nullptr)
{
    ui->setupUi(this);

    setWindowTitle("نظام إدارة الطلاب - HITU | DS");
    resize(1200, 750);
    setMinimumSize(1050, 650);

    setupInterface();
    loadDashboardStats();
    loadStudents();
    loadReports();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupInterface()
{
    setStyleSheet(R"(
        QMainWindow, QWidget#mainRoot {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #17191F, stop:0.52 #20252B, stop:1 #26352F);
        }
        QFrame#topBar { background: rgba(35,42,48,235); border: 1px solid #39444C; border-radius: 16px; }
        QLabel#brandTitle { color: #C7E7D5; font-size: 20px; font-weight: 800; }
        QLabel#brandSubtitle { color: #A7B0B8; font-size: 11px; }
        QPushButton#navButton { color: #D8E0E3; background: transparent; border: 1px solid transparent; border-radius: 10px; padding: 10px 16px; font-size: 13px; }
        QPushButton#navButton:hover { background: #35423D; color: #E8F6EE; border: 1px solid #476354; }
        QPushButton#primaryButton { color: #15221B; background-color: #78C7A1; border: none; border-radius: 10px; padding: 10px 16px; font-weight: bold; }
        QPushButton#primaryButton:hover { background-color: #91D6B4; }
        QPushButton#secondaryButton { color: #E9ECEF; background-color: #303840; border: 1px solid #46515A; border-radius: 10px; padding: 10px 16px; font-weight: bold; }
        QPushButton#secondaryButton:hover { background-color: #39444C; }
        QLabel#pageTitle { color: #D8F0E3; font-size: 27px; font-weight: bold; }
        QLabel#pageSubtitle { color: #A7B0B8; font-size: 13px; }
        QLabel#recordName { color: #C7E7D5; font-size: 22px; font-weight: bold; }
        QFrame#statCard, QFrame#contentCard { background-color: rgba(43,52,59,220); border: 1px solid #3B474F; border-radius: 14px; }
        QLabel#sectionTitle { color: #C7E7D5; font-size: 16px; font-weight: bold; }
        QLabel#statCaption { color: #A7B0B8; font-size: 14px; }
        QLabel#statValue { color: #E9ECEF; font-size: 30px; font-weight: bold; }
        QLineEdit#searchInput { background-color: #303840; border: 1px solid #46515A; border-radius: 10px; padding: 10px 12px; color: #E9ECEF; min-width: 240px; }
        QLineEdit#searchInput:focus { border: 1px solid #78C7A1; }
        QTableWidget { background-color: #262D33; alternate-background-color: #2C343B; border: 1px solid #3B474F; border-radius: 10px; gridline-color: #39444C; color: #E9ECEF; selection-background-color: #3C6553; selection-color: white; }
        QHeaderView::section { background-color: #35423D; color: #D8F0E3; border: none; border-bottom: 1px solid #46515A; padding: 10px 6px; font-weight: bold; }
    )");

    QWidget *root = new QWidget(this);
    root->setObjectName("mainRoot");
    root->setLayoutDirection(Qt::RightToLeft);
    root->setFont(QFont("Segoe UI", 10));
    setCentralWidget(root);

    QVBoxLayout *mainLayout = new QVBoxLayout(root);
    mainLayout->setContentsMargins(22, 18, 22, 18);
    mainLayout->setSpacing(16);

    QFrame *topBar = new QFrame(root);
    topBar->setObjectName("topBar");
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(18, 12, 18, 12);
    topLayout->setSpacing(10);

    QVBoxLayout *brandLayout = new QVBoxLayout;
    QLabel *brand = new QLabel("HITU | DS", topBar);
    brand->setObjectName("brandTitle");
    QLabel *brandSubtitle = new QLabel("نظام السجلات والنتائج الدراسية", topBar);
    brandSubtitle->setObjectName("brandSubtitle");
    brandLayout->addWidget(brand);
    brandLayout->addWidget(brandSubtitle);
    topLayout->addLayout(brandLayout);
    topLayout->addSpacing(18);

    auto makeNavButton = [topBar](const QString &text) {
        auto *button = new QPushButton(text, topBar);
        button->setObjectName("navButton");
        button->setMinimumHeight(42);
        button->setCursor(Qt::PointingHandCursor);
        return button;
    };
    QPushButton *dashboardButton = makeNavButton("الرئيسية");
    QPushButton *studentsButton = makeNavButton("الطلاب");
    QPushButton *reportsButton = makeNavButton("التقارير الأكاديمية");
    topLayout->addWidget(dashboardButton);
    topLayout->addWidget(studentsButton);
    topLayout->addWidget(reportsButton);
    topLayout->addStretch();
    QLabel *roleLabel = new QLabel("حساب الدكتور", topBar);
    roleLabel->setStyleSheet("color:#C7E7D5;background:#35423D;border-radius:10px;padding:8px 12px;");
    topLayout->addWidget(roleLabel);
    mainLayout->addWidget(topBar);

    pages = new QStackedWidget(root);
    pages->addWidget(createDashboardPage());
    pages->addWidget(createStudentsPage());
    pages->addWidget(createAcademicRecordPage());
    pages->addWidget(createReportsPage());
    mainLayout->addWidget(pages, 1);

    connect(dashboardButton, &QPushButton::clicked, this, [this]() {
        loadDashboardStats();
        pages->setCurrentIndex(0);
    });
    connect(studentsButton, &QPushButton::clicked, this, [this]() {
        pages->setCurrentIndex(1);
        loadStudents();
    });
    connect(reportsButton, &QPushButton::clicked, this, [this]() {
        pages->setCurrentIndex(3);
        loadReports();
    });

}

QWidget *MainWindow::createDashboardPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(20);

    QLabel *title = new QLabel("لوحة التحكم", page);
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(
        "نظرة عامة على السجلات والنتائج الدراسية للعام 2025/2026", page
        );
    subtitle->setObjectName("pageSubtitle");

    auto createStatCard = [page](const QString &caption,
                                 QLabel *&valueLabel) -> QFrame * {
        QFrame *card = new QFrame(page);
        card->setObjectName("statCard");
        card->setMinimumHeight(112);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 16, 20, 16);
        cardLayout->setSpacing(8);

        QLabel *captionLabel = new QLabel(caption, card);
        captionLabel->setObjectName("statCaption");

        valueLabel = new QLabel("—", card);
        valueLabel->setObjectName("statValue");

        cardLayout->addWidget(captionLabel);
        cardLayout->addWidget(valueLabel);
        cardLayout->addStretch();
        return card;
    };

    QGridLayout *statsLayout = new QGridLayout;
    statsLayout->setHorizontalSpacing(16);
    statsLayout->setVerticalSpacing(16);

    statsLayout->addWidget(createStatCard("إجمالي الطلاب", dashboardStudentsLabel), 0, 0);
    statsLayout->addWidget(createStatCard("إجمالي المواد", dashboardCoursesLabel), 0, 1);
    statsLayout->addWidget(createStatCard("النتائج المسجلة", dashboardResultsLabel), 1, 0);
    statsLayout->addWidget(createStatCard("طلاب لديهم مواد متبقية", dashboardPendingStudentsLabel), 1, 1);

    QLabel *hint = new QLabel(
        "يتم تحميل الإحصائيات مباشرةً من قاعدة البيانات. "
        "انتقل إلى صفحة الطلاب لاستعراض البيانات والسجل الدراسي لكل طالب.",
        page
        );
    hint->setObjectName("pageSubtitle");
    hint->setWordWrap(true);

    QPushButton *openStudentsButton = new QPushButton("استعراض الطلاب", page);
    openStudentsButton->setObjectName("primaryButton");
    openStudentsButton->setCursor(Qt::PointingHandCursor);
    openStudentsButton->setMaximumWidth(180);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addLayout(statsLayout);
    layout->addWidget(hint);
    layout->addWidget(openStudentsButton);
    layout->addStretch();

    connect(openStudentsButton, &QPushButton::clicked, this, [this]() {
        pages->setCurrentIndex(1);
        loadStudents();
    });

    return page;
}

QWidget *MainWindow::createStudentsPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 26, 30, 26);
    layout->setSpacing(16);

    QLabel *title = new QLabel("إدارة الطلاب", page);
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(
        "استعرض وابحث عن الطلاب، واضغط مرتين على أي طالب لعرض سجله الدراسي.",
        page
        );
    subtitle->setObjectName("pageSubtitle");
    subtitle->setWordWrap(true);

    QHBoxLayout *toolbar = new QHBoxLayout;

    studentSearch = new QLineEdit(page);
    studentSearch->setObjectName("searchInput");
    studentSearch->setPlaceholderText(
        "ابحث بالرقم الجامعي أو الاسم أو رقم الكشف..."
        );

    QPushButton *refreshButton = new QPushButton("تحديث", page);
    refreshButton->setObjectName("secondaryButton");
    refreshButton->setCursor(Qt::PointingHandCursor);

    viewRecordButton = new QPushButton("عرض السجل الدراسي", page);
    viewRecordButton->setObjectName("primaryButton");
    viewRecordButton->setCursor(Qt::PointingHandCursor);
    viewRecordButton->setEnabled(false);

    toolbar->addWidget(studentSearch, 1);
    toolbar->addWidget(refreshButton);
    toolbar->addWidget(viewRecordButton);

    studentsCountLabel = new QLabel("0 طالب", page);
    studentsCountLabel->setObjectName("pageSubtitle");

    studentsTable = new QTableWidget(page);
    studentsTable->setColumnCount(6);
    studentsTable->setHorizontalHeaderLabels({
        "م", "الرقم الجامعي", "اسم الطالب", "الحالة",
        "الحالة قبل الدور الثاني", "المواد المتبقية"
    });
    prepareReadOnlyTable(studentsTable);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addLayout(toolbar);
    layout->addWidget(studentsCountLabel);
    layout->addWidget(studentsTable, 1);

    connect(studentSearch, &QLineEdit::textChanged,
            this, &MainWindow::filterStudents);

    connect(refreshButton, &QPushButton::clicked, this, [this]() {
        loadDashboardStats();
        loadStudents();
        loadReports();
    });

    connect(viewRecordButton, &QPushButton::clicked,
            this, &MainWindow::openSelectedStudentRecord);

    connect(studentsTable, &QTableWidget::itemSelectionChanged,
            this, [this]() {
                if (viewRecordButton) {
                    viewRecordButton->setEnabled(
                        !studentsTable->selectionModel()->selectedRows().isEmpty()
                        );
                }
            });

    connect(studentsTable, &QTableWidget::cellDoubleClicked,
            this, [this](int row, int) {
                if (!studentsTable || !studentsTable->item(row, 1))
                    return;
                showAcademicRecord(studentsTable->item(row, 1)->text());
            });

    return page;
}

QWidget *MainWindow::createAcademicRecordPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 26, 30, 26);
    layout->setSpacing(16);

    QHBoxLayout *topBar = new QHBoxLayout;

    QPushButton *backButton = new QPushButton("العودة إلى الطلاب", page);
    backButton->setObjectName("secondaryButton");
    backButton->setCursor(Qt::PointingHandCursor);

    QLabel *yearLabel = new QLabel("العام الدراسي 2025/2026", page);
    yearLabel->setObjectName("pageSubtitle");

    topBar->addWidget(backButton);
    topBar->addStretch();
    topBar->addWidget(yearLabel);

    QLabel *title = new QLabel("السجل الدراسي", page);
    title->setObjectName("pageTitle");

    recordStudentNameLabel = new QLabel("اسم الطالب", page);
    recordStudentNameLabel->setObjectName("recordName");

    recordStudentInfoLabel = new QLabel("الرقم الجامعي: —", page);
    recordStudentInfoLabel->setObjectName("pageSubtitle");
    recordStudentInfoLabel->setWordWrap(true);

    QFrame *summaryCard = new QFrame(page);
    summaryCard->setObjectName("contentCard");
    QVBoxLayout *summaryLayout = new QVBoxLayout(summaryCard);
    summaryLayout->setContentsMargins(18, 14, 18, 14);

    recordCountLabel = new QLabel("لم يتم تحميل نتائج المواد بعد", summaryCard);
    recordCountLabel->setObjectName("pageSubtitle");
    summaryLayout->addWidget(recordCountLabel);

    recordsTable = new QTableWidget(page);
    recordsTable->setColumnCount(4);
    recordsTable->setHorizontalHeaderLabels({
        "المادة الدراسية", "النهاية العظمى", "رمز التقدير", "التقدير"
    });
    prepareReadOnlyTable(recordsTable);

    layout->addLayout(topBar);
    layout->addWidget(title);
    layout->addWidget(recordStudentNameLabel);
    layout->addWidget(recordStudentInfoLabel);
    layout->addWidget(summaryCard);
    layout->addWidget(recordsTable, 1);

    connect(backButton, &QPushButton::clicked, this, [this]() {
        pages->setCurrentIndex(1);
    });

    return page;
}


QWidget *MainWindow::createReportsPage()
{
    QWidget *page = new QWidget;
    page->setAttribute(Qt::WA_StyledBackground, true);
    page->setStyleSheet("background: transparent;");

    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea(page);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        );
    scrollArea->viewport()->setAutoFillBackground(false);
    scrollArea->viewport()->setStyleSheet(
        "background: transparent; border: none;"
        );

    QWidget *content = new QWidget;
    content->setObjectName("reportsContent");
    content->setAttribute(Qt::WA_StyledBackground, true);
    content->setStyleSheet(
        "QWidget#reportsContent { background: transparent; }"
        );

    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(30, 26, 30, 30);
    layout->setSpacing(16);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    QVBoxLayout *headingLayout = new QVBoxLayout;

    QLabel *title = new QLabel("التقارير الأكاديمية", content);
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(
        "ملخص سنوي وتوزيع التقديرات للعام الدراسي 2025/2026. "
        "جميع الأرقام محسوبة من سجلات قاعدة البيانات المستوردة.",
        content
        );
    subtitle->setObjectName("pageSubtitle");
    subtitle->setWordWrap(true);

    headingLayout->addWidget(title);
    headingLayout->addWidget(subtitle);
    headerLayout->addLayout(headingLayout, 1);

    QPushButton *refreshButton = new QPushButton("تحديث التقارير", content);
    refreshButton->setObjectName("secondaryButton");
    refreshButton->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(refreshButton, 0, Qt::AlignTop);

    layout->addLayout(headerLayout);

    auto createReportCard = [content](const QString &caption,
                                      QLabel *&valueLabel) -> QFrame * {
        QFrame *card = new QFrame(content);
        card->setObjectName("statCard");
        card->setMinimumHeight(105);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 14, 16, 14);
        cardLayout->setSpacing(7);

        QLabel *captionLabel = new QLabel(caption, card);
        captionLabel->setObjectName("statCaption");
        captionLabel->setWordWrap(true);

        valueLabel = new QLabel("—", card);
        valueLabel->setObjectName("statValue");

        cardLayout->addWidget(captionLabel);
        cardLayout->addWidget(valueLabel);

        return card;
    };

    QGridLayout *summaryLayout = new QGridLayout;
    summaryLayout->setHorizontalSpacing(12);
    summaryLayout->setVerticalSpacing(12);

    summaryLayout->addWidget(
        createReportCard("إجمالي سجلات النتائج", reportTotalResultsLabel), 0, 0
        );
    summaryLayout->addWidget(
        createReportCard("التقديرات المعروفة", reportRecognizedGradesLabel), 0, 1
        );
    summaryLayout->addWidget(
        createReportCard("التقديرات الفارغة", reportBlankGradesLabel), 0, 2
        );
    summaryLayout->addWidget(
        createReportCard("تقديرات غير معروفة", reportUnknownGradesLabel), 0, 3
        );

    layout->addLayout(summaryLayout);

    // Grade and student-status summaries
    QHBoxLayout *middleLayout = new QHBoxLayout;
    middleLayout->setSpacing(14);

    QFrame *gradeCard = new QFrame(content);
    gradeCard->setObjectName("contentCard");

    QVBoxLayout *gradeLayout = new QVBoxLayout(gradeCard);
    gradeLayout->setContentsMargins(14, 14, 14, 14);

    QLabel *gradeTitle = new QLabel("توزيع التقديرات", gradeCard);
    gradeTitle->setObjectName("sectionTitle");

    gradeDistributionTable = new QTableWidget(gradeCard);
    gradeDistributionTable->setColumnCount(3);
    gradeDistributionTable->setHorizontalHeaderLabels({
        "الرمز", "التقدير", "العدد"
    });
    prepareReadOnlyTable(gradeDistributionTable);
    gradeDistributionTable->setMinimumHeight(235);

    gradeLayout->addWidget(gradeTitle);
    gradeLayout->addWidget(gradeDistributionTable);

    QFrame *statusCard = new QFrame(content);
    statusCard->setObjectName("contentCard");

    QVBoxLayout *statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(14, 14, 14, 14);

    QLabel *statusTitle = new QLabel("ملخص حالات الطلاب", statusCard);
    statusTitle->setObjectName("sectionTitle");

    studentStatusTable = new QTableWidget(statusCard);
    studentStatusTable->setColumnCount(3);
    studentStatusTable->setHorizontalHeaderLabels({
        "التصنيف", "الحالة", "عدد الطلاب"
    });
    prepareReadOnlyTable(studentStatusTable);
    studentStatusTable->setMinimumHeight(235);

    statusLayout->addWidget(statusTitle);
    statusLayout->addWidget(studentStatusTable);

    middleLayout->addWidget(gradeCard, 1);
    middleLayout->addWidget(statusCard, 1);

    layout->addLayout(middleLayout);

    // Per-course completeness table
    QFrame *courseCard = new QFrame(content);
    courseCard->setObjectName("contentCard");

    QVBoxLayout *courseLayout = new QVBoxLayout(courseCard);
    courseLayout->setContentsMargins(14, 14, 14, 14);

    QLabel *courseTitle = new QLabel("تغطية نتائج المواد", courseCard);
    courseTitle->setObjectName("sectionTitle");

    QLabel *courseSubtitle = new QLabel(
        "من المفترض وجود سجل نتيجة واحد لكل طالب في كل مادة. "
        "يتم احتساب القيم الفارغة والتقديرات غير المعروفة كلٌ على حدة.",
        courseCard
        );
    courseSubtitle->setObjectName("pageSubtitle");
    courseSubtitle->setWordWrap(true);

    courseCoverageTable = new QTableWidget(courseCard);
    courseCoverageTable->setColumnCount(6);
    courseCoverageTable->setHorizontalHeaderLabels({
        "المادة", "النهاية العظمى", "السجلات",
        "المعروف", "الفارغ", "غير المعروف"
    });
    prepareReadOnlyTable(courseCoverageTable);
    courseCoverageTable->setMinimumHeight(430);

    courseLayout->addWidget(courseTitle);
    courseLayout->addWidget(courseSubtitle);
    courseLayout->addWidget(courseCoverageTable);

    layout->addWidget(courseCard);
    layout->addStretch();

    scrollArea->setWidget(content);
    pageLayout->addWidget(scrollArea);

    connect(refreshButton, &QPushButton::clicked,
            this, &MainWindow::loadReports);

    return page;
}


// ==========================================
// Load Dashboard Statistics from SQLite
// ==========================================

void MainWindow::loadDashboardStats()
{
    const QSqlDatabase db = DatabaseManager::database();

    if (!db.isOpen()) {
        qWarning() << "Dashboard statistics: database is not open.";
        return;
    }

    auto loadCount = [this, &db](const QString &sql,
                                 QLabel *targetLabel,
                                 const QString &description) {
        if (!targetLabel)
            return;

        QSqlQuery query(db);
        if (!query.exec(sql) || !query.next()) {
            targetLabel->setText("—");
            qWarning() << "Could not load" << description
                       << query.lastError().text();
            return;
        }

        targetLabel->setText(query.value(0).toString());
        query.finish();
    };

    loadCount("SELECT COUNT(*) FROM students",
              dashboardStudentsLabel, "student count");

    loadCount("SELECT COUNT(*) FROM courses",
              dashboardCoursesLabel, "course count");

    loadCount(
        "SELECT COUNT(*) FROM enrollments e "
        "INNER JOIN semesters s ON s.semester_id = e.semester_id "
        "WHERE s.academic_year = '2025/2026'",
        dashboardResultsLabel, "recorded result count");

    loadCount("SELECT COUNT(*) FROM students WHERE remaining_subjects > 0",
              dashboardPendingStudentsLabel,
              "students with remaining subjects");
}

// ==========================================
// Load Reports from SQLite
// ==========================================

void MainWindow::loadReports()
{
    const QSqlDatabase db = DatabaseManager::database();
    if (!db.isOpen()) {
        QMessageBox::warning(this, "التقارير", "قاعدة البيانات غير مفتوحة.");
        return;
    }

    auto readScalar = [&db](const QString &sql, QString *error) -> int {
        QSqlQuery query(db);
        query.prepare(sql);
        query.bindValue(0, QString::fromLatin1(kAcademicYear));

        if (!query.exec() || !query.next()) {
            if (error)
                *error = query.lastError().text();
            return -1;
        }

        const int value = query.value(0).toInt();
        query.finish();
        return value;
    };

    QString error;

    const int totalResults = readScalar(
        "SELECT COUNT(*) FROM enrollments e "
        "INNER JOIN semesters s ON s.semester_id = e.semester_id "
        "WHERE s.academic_year = ?",
        &error
        );
    if (totalResults < 0) {
        QMessageBox::critical(this, "خطأ في التقارير", error);
        return;
    }

    const int recognizedGrades = readScalar(
        "SELECT COUNT(*) FROM enrollments e "
        "INNER JOIN semesters s ON s.semester_id = e.semester_id "
        "WHERE s.academic_year = ? "
        "AND e.grade_code IS NOT NULL AND TRIM(e.grade_code) <> ''",
        &error
        );
    if (recognizedGrades < 0) {
        QMessageBox::critical(this, "خطأ في التقارير", error);
        return;
    }

    const int blankGrades = readScalar(
        "SELECT COUNT(*) FROM enrollments e "
        "INNER JOIN semesters s ON s.semester_id = e.semester_id "
        "WHERE s.academic_year = ? "
        "AND (e.grade_code IS NULL OR TRIM(e.grade_code) = '') "
        "AND (e.original_grade IS NULL OR TRIM(e.original_grade) = '')",
        &error
        );
    if (blankGrades < 0) {
        QMessageBox::critical(this, "خطأ في التقارير", error);
        return;
    }

    const int unknownGrades = readScalar(
        "SELECT COUNT(*) FROM enrollments e "
        "INNER JOIN semesters s ON s.semester_id = e.semester_id "
        "WHERE s.academic_year = ? "
        "AND (e.grade_code IS NULL OR TRIM(e.grade_code) = '') "
        "AND e.original_grade IS NOT NULL AND TRIM(e.original_grade) <> ''",
        &error
        );
    if (unknownGrades < 0) {
        QMessageBox::critical(this, "خطأ في التقارير", error);
        return;
    }

    reportTotalResultsLabel->setText(QString::number(totalResults));
    reportRecognizedGradesLabel->setText(QString::number(recognizedGrades));
    reportBlankGradesLabel->setText(QString::number(blankGrades));
    reportUnknownGradesLabel->setText(QString::number(unknownGrades));

    // Grade distribution
    gradeDistributionTable->setRowCount(0);
    {
        QSqlQuery query(db);
        query.prepare(R"(
            SELECT
                gt.grade_code,
                gt.grade_name_ar,
                COUNT(e.enrollment_id) AS grade_count
            FROM grade_types gt
            LEFT JOIN enrollments e
                ON e.grade_code = gt.grade_code
                AND e.semester_id IN (
                    SELECT semester_id
                    FROM semesters
                    WHERE academic_year = ?
                )
            GROUP BY gt.grade_code, gt.grade_name_ar
            ORDER BY CASE gt.grade_code
                WHEN 'م' THEN 1
                WHEN 'جـ جـ' THEN 2
                WHEN 'جـ//' THEN 3
                WHEN 'ل' THEN 4
                WHEN 'رل' THEN 5
                WHEN 'ض' THEN 6
                WHEN 'غ' THEN 7
                ELSE 8
            END
        )");
        query.bindValue(0, QString::fromLatin1(kAcademicYear));

        if (!query.exec()) {
            QMessageBox::critical(this, "خطأ في التقارير", query.lastError().text());
            return;
        }

        while (query.next()) {
            const int row = gradeDistributionTable->rowCount();
            gradeDistributionTable->insertRow(row);
            setTableText(gradeDistributionTable, row, 0, query.value(0).toString());
            setTableText(gradeDistributionTable, row, 1, query.value(1).toString());
            setTableText(gradeDistributionTable, row, 2,
                         query.value(2).toString(), true);
        }
        query.finish();
    }

    // Add summary rows for blank and unrecognized source values.
    int row = gradeDistributionTable->rowCount();
    gradeDistributionTable->insertRow(row);
    setTableText(gradeDistributionTable, row, 0, "—");
    setTableText(gradeDistributionTable, row, 1, "تقدير فارغ");
    setTableText(gradeDistributionTable, row, 2,
                 QString::number(blankGrades), true);

    row = gradeDistributionTable->rowCount();
    gradeDistributionTable->insertRow(row);
    setTableText(gradeDistributionTable, row, 0, "—");
    setTableText(gradeDistributionTable, row, 1, "تقدير مصدر غير معروف");
    setTableText(gradeDistributionTable, row, 2,
                 QString::number(unknownGrades), true);

    // Student status and before-second-round status distribution.
    studentStatusTable->setRowCount(0);
    {
        QSqlQuery query(db);
        if (!query.exec(R"(
            SELECT 'Student Status' AS category,
                   COALESCE(NULLIF(TRIM(student_status), ''), 'Not specified') AS status,
                   COUNT(*) AS student_count
            FROM students
            GROUP BY COALESCE(NULLIF(TRIM(student_status), ''), 'Not specified')

            UNION ALL

            SELECT 'Before Second Round' AS category,
                   COALESCE(NULLIF(TRIM(pre_second_round_status), ''), 'Not specified') AS status,
                   COUNT(*) AS student_count
            FROM students
            GROUP BY COALESCE(NULLIF(TRIM(pre_second_round_status), ''), 'Not specified')

            ORDER BY category, student_count DESC, status
        )")) {
            QMessageBox::critical(this, "خطأ في التقارير", query.lastError().text());
            return;
        }

        while (query.next()) {
            const int statusRow = studentStatusTable->rowCount();
            studentStatusTable->insertRow(statusRow);
            const QString rawCategory = query.value(0).toString();
            const QString category = rawCategory == "Student Status"
                                         ? QStringLiteral("حالة الطالب")
                                         : QStringLiteral("الحالة قبل الدور الثاني");
            const QString rawStatus = query.value(1).toString();
            const QString status = rawStatus == "Not specified"
                                       ? QStringLiteral("غير محدد") : rawStatus;
            setTableText(studentStatusTable, statusRow, 0, category);
            setTableText(studentStatusTable, statusRow, 1, status);
            setTableText(studentStatusTable, statusRow, 2,
                         query.value(2).toString(), true);
        }
        query.finish();
    }

    // Course coverage: expected one record per student per course.
    courseCoverageTable->setRowCount(0);
    int totalCourseRecords = 0;
    int totalCourseRecognized = 0;
    int totalCourseBlank = 0;
    int totalCourseUnknown = 0;

    {
        QSqlQuery query(db);
        query.prepare(R"(
            SELECT
                c.course_name,
                c.max_mark,
                COUNT(e.enrollment_id) AS record_count,
                SUM(CASE
                    WHEN e.enrollment_id IS NOT NULL
                     AND e.grade_code IS NOT NULL
                     AND TRIM(e.grade_code) <> ''
                    THEN 1 ELSE 0 END) AS recognized_count,
                SUM(CASE
                    WHEN e.enrollment_id IS NOT NULL
                     AND (e.grade_code IS NULL OR TRIM(e.grade_code) = '')
                     AND (e.original_grade IS NULL OR TRIM(e.original_grade) = '')
                    THEN 1 ELSE 0 END) AS blank_count,
                SUM(CASE
                    WHEN e.enrollment_id IS NOT NULL
                     AND (e.grade_code IS NULL OR TRIM(e.grade_code) = '')
                     AND e.original_grade IS NOT NULL
                     AND TRIM(e.original_grade) <> ''
                    THEN 1 ELSE 0 END) AS unknown_count
            FROM courses c
            LEFT JOIN enrollments e
                ON e.course_id = c.course_id
                AND e.semester_id IN (
                    SELECT semester_id
                    FROM semesters
                    WHERE academic_year = :academicYear
                )
            GROUP BY c.course_id, c.course_name, c.max_mark
            ORDER BY c.course_name
        )");
        query.bindValue(":academicYear", QString::fromLatin1(kAcademicYear));

        if (!query.exec()) {
            QMessageBox::critical(this, "خطأ في التقارير", query.lastError().text());
            return;
        }

        while (query.next()) {
            const int courseRow = courseCoverageTable->rowCount();
            courseCoverageTable->insertRow(courseRow);

            const int records = query.value(2).toInt();
            const int recognized = query.value(3).toInt();
            const int blank = query.value(4).toInt();
            const int unknown = query.value(5).toInt();

            totalCourseRecords += records;
            totalCourseRecognized += recognized;
            totalCourseBlank += blank;
            totalCourseUnknown += unknown;

            setTableText(courseCoverageTable, courseRow, 0, query.value(0).toString());
            setTableText(courseCoverageTable, courseRow, 1,
                         query.value(1).toString(), true);
            setTableText(courseCoverageTable, courseRow, 2,
                         QString::number(records), true);
            setTableText(courseCoverageTable, courseRow, 3,
                         QString::number(recognized), true);
            setTableText(courseCoverageTable, courseRow, 4,
                         QString::number(blank), true);
            setTableText(courseCoverageTable, courseRow, 5,
                         QString::number(unknown), true);
        }
        query.finish();
    }

    // Final totals row makes it easy to reconcile the report with the import summary.
    const int totalRow = courseCoverageTable->rowCount();
    courseCoverageTable->insertRow(totalRow);
    setTableText(courseCoverageTable, totalRow, 0, "الإجمالي");
    setTableText(courseCoverageTable, totalRow, 1, "—", true);
    setTableText(courseCoverageTable, totalRow, 2,
                 QString::number(totalCourseRecords), true);
    setTableText(courseCoverageTable, totalRow, 3,
                 QString::number(totalCourseRecognized), true);
    setTableText(courseCoverageTable, totalRow, 4,
                 QString::number(totalCourseBlank), true);
    setTableText(courseCoverageTable, totalRow, 5,
                 QString::number(totalCourseUnknown), true);

    qInfo() << "Reports refreshed for academic year" << kAcademicYear
            << "Records:" << totalResults
            << "Recognized:" << recognizedGrades
            << "Blank:" << blankGrades
            << "Unrecognized:" << unknownGrades;
}

void MainWindow::loadStudents()
{
    StudentRepository repository;
    QString errorMessage;

    const QList<Student> loadedStudents =
        repository.getAllStudents(&errorMessage);

    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, "خطأ في تحميل الطلاب", errorMessage);
        return;
    }

    studentsCache = loadedStudents;

    if (dashboardStudentsLabel)
        dashboardStudentsLabel->setText(QString::number(studentsCache.size()));

    if (studentSearch)
        filterStudents(studentSearch->text());
}

void MainWindow::filterStudents(const QString &text)
{
    if (!studentsTable)
        return;

    const QString search = text.trimmed().toCaseFolded();

    studentsTable->setSortingEnabled(false);
    studentsTable->setRowCount(0);

    int visibleCount = 0;

    for (const Student &student : studentsCache) {
        const QString roster = QString::number(student.getRosterNumber());

        const bool matches = search.isEmpty()
                             || student.getUniversityId().toCaseFolded().contains(search)
                             || student.getFullName().toCaseFolded().contains(search)
                             || roster.contains(search);

        if (!matches)
            continue;

        const int row = studentsTable->rowCount();
        studentsTable->insertRow(row);

        const QStringList values = {
            student.getRosterNumber() > 0 ? roster : QString(),
            student.getUniversityId(),
            student.getFullName(),
            student.getStatus(),
            student.getPreSecondRoundStatus(),
            QString::number(student.getRemainingSubjects())
        };

        for (int column = 0; column < values.size(); ++column) {
            auto *item = new QTableWidgetItem(values[column]);
            if (column == 0 || column == 5)
                item->setTextAlignment(Qt::AlignCenter);
            studentsTable->setItem(row, column, item);
        }
        ++visibleCount;
    }

    studentsTable->setSortingEnabled(true);

    if (viewRecordButton)
        viewRecordButton->setEnabled(false);

    if (studentsCountLabel) {
        studentsCountLabel->setText(
            QString("عرض %1 من إجمالي %2 طالب")
                .arg(visibleCount)
                .arg(studentsCache.size())
            );
    }
}

void MainWindow::openSelectedStudentRecord()
{
    if (!studentsTable)
        return;

    const QModelIndexList selectedRows =
        studentsTable->selectionModel()->selectedRows();

    if (selectedRows.isEmpty()) {
        QMessageBox::information(this, "اختيار طالب",
                                 "من فضلك اختر طالبًا أولاً.");
        return;
    }

    const int row = selectedRows.first().row();
    if (!studentsTable->item(row, 1))
        return;

    showAcademicRecord(studentsTable->item(row, 1)->text());
}

void MainWindow::showAcademicRecord(const QString &universityId)
{
    const Student *selectedStudent = nullptr;

    for (const Student &student : studentsCache) {
        if (student.getUniversityId() == universityId) {
            selectedStudent = &student;
            break;
        }
    }

    if (!selectedStudent) {
        QMessageBox::warning(
            this, "الطالب غير موجود",
            "تعذر العثور على الطالب المحدد في القائمة الحالية."
            );
        return;
    }

    recordStudentNameLabel->setText(selectedStudent->getFullName());
    recordStudentInfoLabel->setText(
        QString("الرقم الجامعي: %1    |    رقم الكشف: %2    |    الحالة: %3    |    قبل الدور الثاني: %4    |    المواد المتبقية: %5")
            .arg(selectedStudent->getUniversityId())
            .arg(selectedStudent->getRosterNumber())
            .arg(selectedStudent->getStatus())
            .arg(selectedStudent->getPreSecondRoundStatus())
            .arg(selectedStudent->getRemainingSubjects())
        );

    StudentCourseResultRepository repository;
    QString errorMessage;
    const QList<StudentCourseResult> results =
        repository.getResultsForStudent(
            universityId, QString::fromLatin1(kAcademicYear), &errorMessage
            );

    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, "خطأ في السجل الدراسي", errorMessage);
        return;
    }

    recordsTable->setRowCount(0);

    for (const StudentCourseResult &result : results) {
        const int row = recordsTable->rowCount();
        recordsTable->insertRow(row);

        const GradeType &grade = result.getGrade();
        const QString gradeCode = grade.isEmpty()
                                      ? QStringLiteral("—") : grade.getCode();
        const QString gradeDescription = grade.isEmpty()
                                             ? QStringLiteral("لا يوجد تقدير مسجل") : grade.getNameArabic();

        const QStringList values = {
            result.getCourse().getCourseName(),
            QString::number(result.getCourse().getMaxMark()),
            gradeCode,
            gradeDescription
        };

        for (int column = 0; column < values.size(); ++column) {
            auto *item = new QTableWidgetItem(values[column]);
            if (column == 1)
                item->setTextAlignment(Qt::AlignCenter);
            recordsTable->setItem(row, column, item);
        }
    }

    recordCountLabel->setText(
        QString("عدد المواد المسجلة: %1 للعام الدراسي 2025/2026")
            .arg(results.size())
        );
    pages->setCurrentIndex(2);
}
