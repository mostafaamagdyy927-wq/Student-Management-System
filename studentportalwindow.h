#pragma once
#include <QMainWindow>
class QLabel; class QTableWidget;
class StudentPortalWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit StudentPortalWindow(const QString &universityId, QWidget *parent = nullptr);
private:
    void loadStudent();
    QString m_universityId;
    QLabel *nameLabel; QLabel *infoLabel; QLabel *statusLabel; QLabel *countLabel;
    QTableWidget *resultsTable;
};
