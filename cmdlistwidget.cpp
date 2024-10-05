#include "cmdlistwidget.h"
#include <QKeyEvent>
#include <QApplication>
#include <QtConcurrent>
#include <QTime>
#include <Utils/WinUtility.h>
#include <Utils/WinUtility.h>
#include <Utils/cacheiconprovider.h>

CMDListWidget::CMDListWidget(QWidget* parent)
    : QListWidget(parent)
{
    setStyleSheet("QListWidget{background-color:rgb(15,15,15);color:rgb(200,200,200);border:none;outline:0px;}" //透明背景会导致边框闪现
                  "QListWidget::item:selected{background-color:lightgray; color:black; }"
                  "QListWidget::item:selected:!active{background-color:gray; }");
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setUniformItemSizes(true); //统一优化

    connect(this, &CMDListWidget::itemDoubleClicked, this, &CMDListWidget::itemActivedEx);

    connect(this, &CMDListWidget::iconReady, this, [=](QListWidgetItem* item, int index, QIcon icon) { //信号与槽，线程安全
        if (item && this->count() > index && item == this->item(index) && !icon.isNull()) {
            item->setIcon(icon); // GUI操作最好在主线程进行
        }
    });
}

void CMDListWidget::addIconItems(const IconStrList& list) //貌似加载不同的图标会降低绘制速度（可能影响优化）
{
    static QReadWriteLock lock;
    static QTime tModify;
    {
        QWriteLocker locker(&lock);
        if (listCache == list) { //防止重复绘制
            qDebug() << "Hit Cache";
            return;
        }
        tModify = QTime::currentTime(); //上次被修改的时间
        listCache = list;

        this->clear();
        for (auto& p : list) {
            static QIcon nullIcon(":/images/web.png"); //默认图标
            QListWidgetItem* item = new QListWidgetItem(p.second); // 这里指定了parent的话，会自动插入指定list
            item->setSizeHint(QSize(0, Item_H));
            //item->setIcon(p.first); //QIcon()==无图标
            // if (!p.first.isNull())
            if (!p.first.isEmpty())
                item->setIcon(nullIcon);
            addItem(item);
        }
    }
    setCurrentRow(0);
    adjustSizeEx();

    // QTimer::singleShot(0, this, [=]() { //进入事件队列 在首次渲染list完成后再add Icon
        QtConcurrent::run([this]() { // 或者可以每一个item都开一个线程
            //qApp->processEvents();
            ComInitializer COM; //每个线程都要单独初始化COM
            QTime t;
            int rows;
            {
                QReadLocker locker(&lock);
                t = tModify; //static 无需捕获
                rows = count();
            }
            for (int i = 0; i < rows; i++) {
                if (!isVisible()) return;
                QString iconPath;
                QListWidgetItem* item = nullptr;
                {
                    QReadLocker locker(&lock);
                    if (t != tModify) return; //listCache被修改
                    if (listCache.size() != rows) return; //防止hideEvent清空listCache

                    iconPath = listCache[i].first;
                    item = this->item(i);
                }
                Win::getFileIcon(iconPath); //预加载，系统级缓存，这是最耗时的操作 (QIcon::pixmap)
                auto icon = CacheIconProvider::instance().icon(iconPath); //大概应该可以在非GUI线程调用？（updateAppList中用了）
                emit iconReady(item, i, icon);

                // icon.pixmap(16, 16); //获取使其强制缓存 (在后台线程貌似会卡死)
                // item(i)->setIcon(listCache[i].first); //setIcon不耗时，渲染耗时
                // qApp->processEvents(); //假如list为0，会触发hideEvent，导致listCache清空
            }
        });
    // });
}

void CMDListWidget::adjustSizeEx()
{
    int rows = count();
    int maxW = 0;
    QFontMetrics fontMetrics(font());
    for (int i = 0; i < rows; i++)
        maxW = std::max(maxW, fontMetrics.horizontalAdvance(item(i)->text()));
    resize(maxW + 2 * frameWidth() + Item_H + 15, Item_H * rows + 2 * frameWidth());
}

void CMDListWidget::selectNext()
{
    int i = currentRow() + 1;
    setCurrentRow(i % count());
}

void CMDListWidget::selectPre()
{
    int i = currentRow() - 1;
    setCurrentRow((i + count()) % count()); //循环
}

QString CMDListWidget::selectedText()
{
    return currentItem()->text();
}

void CMDListWidget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit itemActivedEx(currentItem());
        break;
    default:
        break;
    }
    return QListWidget::keyPressEvent(event);
}

void CMDListWidget::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() > 0)
        selectPre();
    else
        selectNext();
    return QListWidget::wheelEvent(event);
}

void CMDListWidget::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event)
    // 这里没有去更新tModify，bomb
    listCache.clear(); //hide时Cache失效，防止show时 Cache==list 导致不绘制
}

bool operator==(const QIcon& lhs, const QIcon& rhs) //QIcon只能用cacheKey 无预定义==
{
    return lhs.cacheKey() == rhs.cacheKey();
}
