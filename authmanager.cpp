#include "authmanager.h"
#include "databasemanager.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

QString AuthManager::makeSalt()
{
    QByteArray bytes;
    for (int i = 0; i < 4; ++i)
        bytes.append(QByteArray::number(QRandomGenerator::global()->generate64(), 16));
    return QString::fromLatin1(bytes.toHex());
}

QString AuthManager::hashPassword(const QString &password, const QString &salt)
{
    QByteArray value = (salt + password).toUtf8();
    // Iterated SHA-256 with a per-account salt. (No plaintext is stored.)
    for (int i = 0; i < 12000; ++i)
        value = QCryptographicHash::hash(value, QCryptographicHash::Sha256);
    return QString::fromLatin1(value.toHex());
}

bool AuthManager::initialize(QString *errorMessage)
{
    QSqlDatabase db = DatabaseManager::database();
    QSqlQuery q(db);
    if (!q.exec(R"(CREATE TABLE IF NOT EXISTS user_accounts (
        account_id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT NOT NULL UNIQUE,
        role TEXT NOT NULL CHECK(role IN ('doctor','student')),
        university_id TEXT UNIQUE,
        password_salt TEXT NOT NULL,
        password_hash TEXT NOT NULL,
        must_change_password INTEGER NOT NULL DEFAULT 1,
        created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(university_id) REFERENCES students(university_id) ON DELETE CASCADE
    ))")) {
        if (errorMessage) *errorMessage = q.lastError().text();
        return false;
    }

    // Create the initial doctor account only when no doctor account exists.
    q.prepare("SELECT COUNT(*) FROM user_accounts WHERE role='doctor'");
    if (!q.exec() || !q.next()) {
        if (errorMessage) *errorMessage = q.lastError().text();
        return false;
    }
    const bool noDoctor = q.value(0).toInt() == 0;
    q.finish();
    if (noDoctor) {
        const QString salt = makeSalt();
        q.prepare("INSERT INTO user_accounts(username,role,password_salt,password_hash,must_change_password) VALUES(?,?,?,?,1)");
        q.addBindValue("admin"); q.addBindValue("doctor"); q.addBindValue(salt);
        q.addBindValue(hashPassword("admin123", salt));
        if (!q.exec()) { if (errorMessage) *errorMessage = q.lastError().text(); return false; }
    }

    // Give every existing student an account. Initial password: university ID + "ds".
    q.exec("SELECT university_id FROM students WHERE university_id IS NOT NULL AND TRIM(university_id)<>''");
    while (q.next()) {
        const QString id = q.value(0).toString();
        QSqlQuery check(db);
        check.prepare("SELECT COUNT(*) FROM user_accounts WHERE username=?");
        check.addBindValue(id);
        if (!check.exec() || !check.next()) { if (errorMessage) *errorMessage = check.lastError().text(); return false; }
        if (check.value(0).toInt() == 0) {
            const QString salt = makeSalt();
            QSqlQuery add(db);
            add.prepare("INSERT INTO user_accounts(username,role,university_id,password_salt,password_hash,must_change_password) VALUES(?,?,?,?,?,1)");
            add.addBindValue(id); add.addBindValue("student"); add.addBindValue(id); add.addBindValue(salt);
            add.addBindValue(hashPassword(id + "ds", salt));
            if (!add.exec()) { if (errorMessage) *errorMessage = add.lastError().text(); return false; }
        }
    }
    q.finish();
    return true;
}

bool AuthManager::authenticate(const QString &username, const QString &password,
                               AuthSession *session, bool *mustChangePassword,
                               QString *errorMessage)
{
    QSqlQuery q(DatabaseManager::database());
    q.prepare("SELECT role, university_id, password_salt, password_hash, must_change_password FROM user_accounts WHERE username=?");
    q.addBindValue(username.trimmed());
    if (!q.exec()) { if (errorMessage) *errorMessage = q.lastError().text(); return false; }
    if (!q.next()) { if (errorMessage) *errorMessage = QStringLiteral("اسم المستخدم أو كلمة المرور غير صحيحة."); return false; }
    const QString role = q.value(0).toString();
    const QString universityId = q.value(1).toString();
    const QString salt = q.value(2).toString();
    const QString expected = q.value(3).toString();
    const bool changeRequired = q.value(4).toBool();
    q.finish();
    if (hashPassword(password, salt) != expected) {
        if (errorMessage) *errorMessage = QStringLiteral("اسم المستخدم أو كلمة المرور غير صحيحة.");
        return false;
    }
    if (session) { session->role = role; session->username = username.trimmed(); session->universityId = universityId; }
    if (mustChangePassword) *mustChangePassword = changeRequired;
    return true;
}

bool AuthManager::changePassword(const QString &username, const QString &newPassword, QString *errorMessage)
{
    if (newPassword.trimmed().size() < 8) {
        if (errorMessage) *errorMessage = QStringLiteral("كلمة المرور لازم تكون 8 أحرف على الأقل.");
        return false;
    }
    const QString salt = makeSalt();
    QSqlQuery q(DatabaseManager::database());
    q.prepare("UPDATE user_accounts SET password_salt=?, password_hash=?, must_change_password=0 WHERE username=?");
    q.addBindValue(salt); q.addBindValue(hashPassword(newPassword, salt)); q.addBindValue(username);
    if (!q.exec() || q.numRowsAffected() != 1) { if (errorMessage) *errorMessage = q.lastError().text(); return false; }
    return true;
}
