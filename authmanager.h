#pragma once
#include <QString>

struct AuthSession {
    QString role;          // "doctor" or "student"
    QString username;
    QString universityId;  // populated for student sessions
};

class AuthManager final {
public:
    static bool initialize(QString *errorMessage = nullptr);
    static bool authenticate(const QString &username, const QString &password,
                             AuthSession *session, bool *mustChangePassword,
                             QString *errorMessage = nullptr);
    static bool changePassword(const QString &username, const QString &newPassword,
                              QString *errorMessage = nullptr);
private:
    static QString makeSalt();
    static QString hashPassword(const QString &password, const QString &salt);
};
