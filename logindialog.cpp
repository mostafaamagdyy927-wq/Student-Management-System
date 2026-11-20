
#include "logindialog.h"
#include "authmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPixmap>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent),
    usernameEdit(nullptr),
    passwordEdit(nullptr),
    messageLabel(nullptr)
{
    setWindowTitle(QStringLiteral("تسجيل الدخول | HITU DS"));
    setFixedSize(520, 560);
    setLayoutDirection(Qt::RightToLeft);

    setStyleSheet(R"(
        QDialog {
            background: qlineargradient(
                x1:0,y1:0,x2:1,y2:1,
                stop:0 #17191F,
                stop:0.55 #20252B,
                stop:1 #26352F
            );
        }

        QLabel {
            color:#E9ECEF;
        }

        QLabel#logo {
            font-size:30px;
            font-weight:800;
            color:#C7E7D5;
        }

        QLabel#muted {
            color:#A7B0B8;
            font-size:13px;
        }

        QLineEdit {
            background:#303840;
            color:#F3F6F7;
            border:1px solid #46515A;
            border-radius:12px;
            padding:13px;
            font-size:15px;
        }

        QLineEdit:focus {
            border:1px solid #78C7A1;
        }

        QPushButton {
            background:#78C7A1;
            color:#15221B;
            font-weight:bold;
            border:0;
            border-radius:12px;
            padding:13px;
            font-size:15px;
        }

        QPushButton:hover {
            background:#91D6B4;
        }
    )");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(42, 30, 42, 25);
    layout->setSpacing(12);

    auto *logo = new QLabel("HITU  |  DS", this);
    logo->setObjectName("logo");
    logo->setAlignment(Qt::AlignCenter);

    auto *heading = new QLabel("بوابتك الأكاديمية", this);
    heading->setAlignment(Qt::AlignCenter);
    heading->setStyleSheet("font-size:23px;font-weight:700;");

    auto *sub = new QLabel("سجّل الدخول للمتابعة", this);
    sub->setObjectName("muted");
    sub->setAlignment(Qt::AlignCenter);

    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("اسم المستخدم / الرقم الجامعي");
    usernameEdit->setMinimumHeight(48);

    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("كلمة المرور");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMinimumHeight(48);

    auto *login = new QPushButton("دخول", this);
    login->setMinimumHeight(48);

    messageLabel = new QLabel(this);
    messageLabel->setStyleSheet("color:#FFB4A9;");
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);

    auto *hint = new QLabel(
        "حساب الدكتور: admin  |  كلمة المرور الأولية: admin123\n"
        "حساب الطالب: الرقم الجامعي  |  كلمة المرور الأولية: الرقم الجامعي + ds\n"
        "سيُطلب تغيير كلمة المرور عند أول دخول.",
        this
        );

    hint->setObjectName("muted");
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);

    // صورة أسفل فورم تسجيل الدخول
    auto *imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setFixedHeight(105);
    imageLabel->setStyleSheet("background:transparent;");

    QPixmap image(":/images/img.webp");

    if (!image.isNull()) {
        imageLabel->setPixmap(
            image.scaled(
                400,
                100,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                )
            );
    }

    layout->addWidget(logo);
    layout->addSpacing(5);
    layout->addWidget(heading);
    layout->addWidget(sub);
    layout->addSpacing(8);

    layout->addWidget(usernameEdit);
    layout->addWidget(passwordEdit);
    layout->addWidget(login);
    layout->addWidget(messageLabel);

    layout->addStretch();

    layout->addWidget(hint);
    layout->addWidget(imageLabel);

    connect(login, &QPushButton::clicked,
            this, &LoginDialog::attemptLogin);

    connect(passwordEdit, &QLineEdit::returnPressed,
            this, &LoginDialog::attemptLogin);

    usernameEdit->setFocus();
}

bool LoginDialog::forcePasswordChange(const QString &username)
{
    QDialog dlg(this);
    dlg.setWindowTitle("تغيير كلمة المرور");
    dlg.setFixedWidth(420);
    dlg.setLayoutDirection(Qt::RightToLeft);

    dlg.setStyleSheet(
        "QDialog{background:#20252B;color:#E9ECEF;}"
        "QLabel{color:#E9ECEF;}"
        "QLineEdit{background:#303840;color:white;"
        "border:1px solid #46515A;border-radius:8px;padding:10px;}"
        "QPushButton{background:#78C7A1;color:#15221B;"
        "border:0;border-radius:8px;padding:10px;font-weight:bold;}"
        );

    auto *l = new QVBoxLayout(&dlg);

    l->addWidget(new QLabel(
        "يجب تغيير كلمة المرور الافتراضية قبل المتابعة.",
        &dlg
        ));

    auto *p1 = new QLineEdit(&dlg);
    p1->setEchoMode(QLineEdit::Password);
    p1->setPlaceholderText("كلمة المرور الجديدة (8 أحرف على الأقل)");

    auto *p2 = new QLineEdit(&dlg);
    p2->setEchoMode(QLineEdit::Password);
    p2->setPlaceholderText("تأكيد كلمة المرور");

    auto *err = new QLabel(&dlg);
    err->setStyleSheet("color:#FFB4A9;");

    auto *save = new QPushButton("حفظ كلمة المرور", &dlg);

    l->addWidget(p1);
    l->addWidget(p2);
    l->addWidget(err);
    l->addWidget(save);

    connect(save, &QPushButton::clicked,
            &dlg, [&dlg, p1, p2, err, username]() {

                if (p1->text() != p2->text()) {
                    err->setText("كلمتا المرور غير متطابقتين.");
                    return;
                }

                QString error;

                if (!AuthManager::changePassword(username, p1->text(), &error)) {
                    err->setText(error);
                    return;
                }

                dlg.accept();
            });

    return dlg.exec() == QDialog::Accepted;
}

void LoginDialog::attemptLogin()
{
    AuthSession candidate;
    bool mustChange = false;
    QString error;

    if (!AuthManager::authenticate(
            usernameEdit->text(),
            passwordEdit->text(),
            &candidate,
            &mustChange,
            &error)) {

        messageLabel->setText(error);
        return;
    }

    if (mustChange && !forcePasswordChange(candidate.username)) {
        messageLabel->setText(
            "لا يمكن المتابعة دون تغيير كلمة المرور."
            );
        return;
    }

    m_session = candidate;
    accept();
}
