#ifndef PROJECTHEADERITEM_H
#define PROJECTHEADERITEM_H
#include "taskboard.h"
#include <QGraphicsRectItem>

class ProjectHeaderItem : public QGraphicsRectItem
{
public:
    ProjectHeaderItem(int projectIdx, TaskBoardModel *model, QGraphicsItem *parent = nullptr);
    void updatePosition(double y);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    int projectId;
    TaskBoardModel *model;
    QGraphicsTextItem *label;
};

#endif // PROJECTHEADERITEM_H
