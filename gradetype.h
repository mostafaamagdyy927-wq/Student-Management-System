
#pragma once

#include <QString>

class GradeType
{
private:
    QString code;
    QString nameArabic;

    GradeType(const QString &code, const QString &nameArabic);

public:
    GradeType();

    static GradeType fromCode(const QString &code);

    QString getCode() const;
    QString getNameArabic() const;

    bool isEmpty() const;
    bool isValid() const;
};
