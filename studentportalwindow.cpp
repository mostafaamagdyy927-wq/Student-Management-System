
#include "studentportalwindow.h"
#include "databasemanager.h"
#include "studentcourseresultrepository.h"
#include "studentcourseresult.h"
#include "gradetype.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStringList>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QSqlQuery>
#include <QMessageBox>

namespace
{
const QString kYear = QStringLiteral("2025/2026");

// Current student information
const QString kDepartment =
    QStringLiteral("علوم البيانات (Data Science)");

const QString kAcademicLevel =
    QStringLiteral("الفرقة الرابعة");
}

StudentPortalWindow::StudentPortalWindow(
    const QString &universityId,
    QWidget *parent
    )
    : QMainWindow(parent),
    m_universityId(universityId),
    nameLabel(nullptr),
    infoLabel(nullptr),
    statusLabel(nullptr),
    countLabel(nullptr),
    resultsTable(nullptr)
{
    setWindowTitle("بوابة الطالب | HITU DS");
    resize(1120, 760);
    setMinimumSize(900, 620);

    setStyleSheet(R"(
        QMainWindow,
        QWidget#root {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #17191F,
                stop:0.52 #20252B,
                stop:1 #26352F
            );
        }

        QLabel {
            color: #E9ECEF;
        }

        QLabel#title {
            font-size: 27px;
            font-weight: 800;
            color: #D8F0E3;
        }

        QLabel#muted {
            color: #A7B0B8;
            font-size: 13px;
        }

        QFrame#card {
            background: rgba(43,52,59,220);
            border: 1px solid #3B474F;
            border-radius: 16px;
        }

        QLabel#value {
            font-size: 20px;
            font-weight: bold;
            color: #C7E7D5;
        }

        QTableWidget {
            background: #262D33;
            alternate-background-color: #2C343B;
            color: #E9ECEF;
            border: 1px solid #3B474F;
            border-radius: 12px;
            gridline-color: #39444C;
            selection-background-color: #3C6553;
            selection-color: white;
        }

        QHeaderView::section {
            background: #35423D;
            color: #D8F0E3;
            border: 0;
            padding: 11px;
            font-weight: bold;
        }

        QPushButton {
            background: #78C7A1;
            color: #15221B;
            border: 0;
            border-radius: 10px;
            padding: 10px 18px;
            font-weight: bold;
        }
    )");

    auto *root = new QWidget(this);
    root->setObjectName("root");
    root->setLayoutDirection(Qt::RightToLeft);
    setCentralWidget(root);

    auto *layout = new QVBoxLayout(root);
    layout->setContentsMargins(30, 28, 30, 28);
    layout->setSpacing(18);

    // Header
    auto *header = new QHBoxLayout();

    auto *brand = new QLabel("HITU  |  DS", root);
    brand->setStyleSheet(
        "font-size:18px;font-weight:800;color:#C7E7D5;"
        );

    auto *heading = new QLabel("بوابة الطالب", root);
    heading->setObjectName("title");

    auto *logout = new QPushButton("تسجيل الخروج", root);
    logout->setStyleSheet(
        "background:#39434A;color:#E9ECEF;"
        );

    connect(
        logout,
        &QPushButton::clicked,
        this,
        &QWidget::close
        );

    header->addWidget(brand);
    header->addStretch();
    header->addWidget(heading);
    header->addStretch();
    header->addWidget(logout);

    layout->addLayout(header);

    // Student information card
    auto *card = new QFrame(root);
    card->setObjectName("card");

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 20);
    cardLayout->setSpacing(10);

    nameLabel = new QLabel("...", card);
    nameLabel->setObjectName("title");

    infoLabel = new QLabel(card);
    infoLabel->setObjectName("muted");

    statusLabel = new QLabel(card);
    statusLabel->setObjectName("value");

    cardLayout->addWidget(nameLabel);
    cardLayout->addWidget(infoLabel);
    cardLayout->addWidget(statusLabel);

    layout->addWidget(card);

    // Academic record header
    auto *recordsHead = new QHBoxLayout();

    auto *recordsTitle =
        new QLabel("السجل الأكاديمي", root);

    recordsTitle->setObjectName("title");

    countLabel =
        new QLabel("العام الدراسي " + kYear, root);

    countLabel->setObjectName("muted");

    recordsHead->addWidget(recordsTitle);
    recordsHead->addStretch();
    recordsHead->addWidget(countLabel);

    layout->addLayout(recordsHead);

    // Results table
    resultsTable = new QTableWidget(root);

    resultsTable->setColumnCount(4);

    resultsTable->setHorizontalHeaderLabels({
        "المادة",
        "النهاية العظمى",
        "رمز التقدير",
        "التقدير"
    });

    resultsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    resultsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
        );

    resultsTable->setSelectionMode(
        QAbstractItemView::SingleSelection
        );

    resultsTable->verticalHeader()->hide();

    resultsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
        );

    resultsTable->horizontalHeader()->setStretchLastSection(true);

    resultsTable->setLayoutDirection(Qt::RightToLeft);

    layout->addWidget(resultsTable, 1);

    // Footer
    auto *footer = new QLabel(
        "هذه البوابة تعرض بيانات حسابك فقط.",
        root
        );

    footer->setObjectName("muted");
    footer->setAlignment(Qt::AlignCenter);

    layout->addWidget(footer);

    loadStudent();
}

void StudentPortalWindow::loadStudent()
{
    QSqlQuery q(DatabaseManager::database());

    q.prepare(R"(
        SELECT
            full_name,
            roster_number,
            student_status,
            pre_second_round_status,
            remaining_subjects
        FROM students
        WHERE university_id = ?
    )");

    q.addBindValue(m_universityId);

    if (!q.exec() || !q.next()) {
        QMessageBox::critical(
            this,
            "تعذر تحميل البيانات",
            "لم يتم العثور على بيانات الطالب."
            );

        close();
        return;
    }

    nameLabel->setText(q.value(0).toString());

    // Display current department and academic level.
    // Previous academic results remain unchanged.
    infoLabel->setText(
        QString(
            "الرقم الجامعي: %1  |  رقم الكشف: %2  |  "
            "القسم: %3  |  الفرقة: %4"
            ).arg(
                m_universityId,
                q.value(1).toString(),
                kDepartment,
                kAcademicLevel
                )
        );

    statusLabel->setText(
        QString(
            "الحالة: %1  |  قبل الدور الثاني: %2  |  "
            "المواد المتبقية: %3"
            ).arg(
                q.value(2).toString(),
                q.value(3).toString(),
                q.value(4).toString()
                )
        );

    q.finish();

    // Load existing academic results without modifying them.
    StudentCourseResultRepository repo;

    QString error;

    const auto results =
        repo.getResultsForStudent(
            m_universityId,
            kYear,
            &error
            );

    if (!error.isEmpty()) {
        QMessageBox::warning(
            this,
            "خطأ في النتائج",
            error
            );

        return;
    }

    resultsTable->setRowCount(0);

    for (const auto &result : results) {

        const int row = resultsTable->rowCount();

        resultsTable->insertRow(row);

        const auto grade = result.getGrade();

        QStringList values = {
            result.getCourse().getCourseName(),
            QString::number(
                result.getCourse().getMaxMark()
                ),
            grade.isEmpty()
                ? QStringLiteral("—")
                : grade.getCode(),
            grade.isEmpty()
                ? QStringLiteral("لا يوجد تقدير مسجل")
                : grade.getNameArabic()
        };

        for (int c = 0; c < values.size(); ++c) {

            auto *item =
                new QTableWidgetItem(values[c]);

            if (c > 0) {
                item->setTextAlignment(Qt::AlignCenter);
            }

            resultsTable->setItem(row, c, item);
        }
    }

    countLabel->setText(
        QString("العام الدراسي %1  |  %2 مادة")
            .arg(kYear)
            .arg(results.size())
        );
}
