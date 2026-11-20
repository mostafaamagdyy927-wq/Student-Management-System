
#include "gradetype.h"

GradeType::GradeType()
{
}

GradeType::GradeType(
    const QString &code,
    const QString &nameArabic
    )
    : code(code),
    nameArabic(nameArabic)
{
}

GradeType GradeType::fromCode(const QString &code)
{
    const QString value = code.trimmed();

    if (value == QStringLiteral("م"))
        return GradeType(value, QStringLiteral("ممتاز"));

    if (value == QStringLiteral("جـ جـ"))
        return GradeType(value, QStringLiteral("جيد جدًا"));

    if (value == QStringLiteral("جـ//"))
        return GradeType(value, QStringLiteral("جيد"));

    if (value == QStringLiteral("ل"))
        return GradeType(value, QStringLiteral("مقبول"));

    if (value == QStringLiteral("رل"))
        return GradeType(value, QStringLiteral("راسب لائحة"));

    if (value == QStringLiteral("ض"))
        return GradeType(value, QStringLiteral("ضعيف"));

    if (value == QStringLiteral("غ"))
        return GradeType(value, QStringLiteral("غياب"));

    return GradeType(value, QString());
}

QString GradeType::getCode() const
{
    return code;
}

QString GradeType::getNameArabic() const
{
    return nameArabic;
}

bool GradeType::isEmpty() const
{
    return code.isEmpty();
}

bool GradeType::isValid() const
{
    return !code.isEmpty() && !nameArabic.isEmpty();
}
