#include "mainwindow.h"
#include "databasemanager.h"
#include "authmanager.h"
#include "logindialog.h"
#include "studentportalwindow.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("StudentManagementSystem");
    app.setApplicationName("StudentManagementSystem");

    QString databaseError;

    if (!DatabaseManager::initialize(&databaseError)) {
        QMessageBox::critical(
            nullptr,
            "Database Error",
            QString("Could not initialize database:\n") + databaseError
            );
        return 1;
    }

    QString authError;
    if (!AuthManager::initialize(&authError)) {
        QMessageBox::critical(nullptr, "Authentication Error", "Could not initialize accounts:\n" + authError);
        return 1;
    }

    LoginDialog login;
    if (login.exec() != QDialog::Accepted)
        return 0;

    const AuthSession session = login.session();
    if (session.role == "doctor") {
        MainWindow window;
        window.show();
        return app.exec();
    }

    StudentPortalWindow window(session.universityId);
    window.show();
    return app.exec();
}
