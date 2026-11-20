#pragma once
#include <QDialog>
#include "authmanager.h"
class QLineEdit;
class QLabel;

class LoginDialog final : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    AuthSession session() const { return m_session; }
private slots:
    void attemptLogin();
private:
    bool forcePasswordChange(const QString &username);
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLabel *messageLabel;
    AuthSession m_session;
};
