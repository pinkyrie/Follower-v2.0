#ifndef CACHEICONPROVIDER_H
#define CACHEICONPROVIDER_H

#include <QFileIconProvider>
#include <QObject>
#include <QMap>
class CacheIconProvider : public QFileIconProvider { //非QObject子类不能用Q_OBJECT宏
public:
    static CacheIconProvider& instance();
    QIcon getUrlIcon(const QString& path);
    void addCache(const QString& path);
    QIcon icon(const QString& path); //cacheIcon

private:
    CacheIconProvider();

private:
    QMap<QString, QIcon> iconCache;
};

#endif // CACHEICONPROVIDER_H
