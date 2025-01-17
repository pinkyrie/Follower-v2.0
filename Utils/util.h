#ifndef UTIL_H
#define UTIL_H

#include <QString>


class Util
{
private:
    Util() = delete;

public:
    // 判断是否可能是路径，例如'C:' 'C:\'
    static bool maybePath(const QString& str);
    static bool hasChinese(const QString &str);
    static bool hasVisibleWidget(void);
};

#endif // UTIL_H
