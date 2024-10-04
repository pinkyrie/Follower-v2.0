#include "util.h"

#include <QCache>

bool Util::maybePath(const QString& str)
{
    if (str.size() < 2) return false;
    return str[0].isLetter() && str[1] == ':';
}

// 判断 QString 是否包含中文汉字
bool Util::hasChinese(const QString &str) {
    static QCache<QString, bool> cache; // 100
    if (cache.contains(str)) return *cache[str];

    bool res = false;
    for (const QChar &ch : str) {
        if (ch.script() == QChar::Script_Han) { // 正则表达式判断unicode范围貌似不行
            res = true;
            break;
        }
    }

    cache.insert(str, new bool(res));
    return res;
}
