#ifndef PROJECTHEADERITEM_H
#define PROJECTHEADERITEM_H
#include "taskboard.h"
#include <QGraphicsRectItem>
#include <QPainter>

class ProjectHeaderItem : public QGraphicsRectItem
{
public:
    ProjectHeaderItem(int projectIdx, TaskBoardModel *model, QGraphicsItem *parent = nullptr);
    void updatePosition(double y);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
    int projectId;
    TaskBoardModel *model;
};

#endif // PROJECTHEADERITEM_H
