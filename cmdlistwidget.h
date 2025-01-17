#ifndef CMDLISTWIDGET_H
#define CMDLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QList>
#include "Utils/systemapi.h"
class CMDListWidget : public QListWidget
{
    Q_OBJECT
public:
    using IconStrList = QList<QPair<QString, QString>>; // iconPath, text

    CMDListWidget(QWidget* parent = nullptr);
    void addIconItems(const IconStrList& list);
    void adjustSizeEx(void);
    void selectNext(void);
    void selectPre(void);
    QString selectedText(void);

private:
    const int Item_H = DPI(25);
    IconStrList listCache;

signals:
    void itemActivedEx(QListWidgetItem* item); //双击或回车
    void iconReady(QListWidgetItem* item, int index, QIcon icon); //icon加载完成

    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    // QWidget interface
protected:
    void hideEvent(QHideEvent* event) override;
};

bool operator==(const QIcon& lhs, const QIcon& rhs); //定义运算符

#endif // CMDLISTWIDGET_H
