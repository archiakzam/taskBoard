#ifndef GRAPHICSTASKITEM_H
#define GRAPHICSTASKITEM_H

#include <QGraphicsRectItem>
#include <QCursor>
#include <QDateTime>
#include "taskboard.h"
#include<QPainter>
#include"timelineview.h"

class TaskBoardModel;
class TimelineView;

class GraphicsTaskItem : public QGraphicsRectItem {
public:
    enum ResizeHandle { NoHandle, LeftHandle, RightHandle };

    GraphicsTaskItem(const Task &task, int projId, int taskId, TaskBoardModel *model,
                     TimelineView *view, bool isTemporary = false, QGraphicsItem *parent = nullptr);

    void updateGeometry(const QDateTime &sceneStart, double pixelsPerHour);
    void setViewParams(const QDateTime &start, double pph) {
        sceneStartTime = start;
        pixelsPerHour = pph;
    }

    bool isTemporary() const { return temporary; }
    void commitTask();
    void setTemporary(bool temp);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    QPainterPath shape() const override;

private:
    ResizeHandle handleAt(const QPointF &pos) const;
    void setCursorForHandle(ResizeHandle handle);
    bool isWithinDayBoundary(const QDateTime &start, const QDateTime &end) const;

    Task taskData;
    int projectId;
    int taskId;
    TaskBoardModel *model;
    TimelineView *timelineView;
    QRectF lastRect;

    ResizeHandle activeHandle;
    QPointF dragStartPos;
    QDateTime originalStart;
    QDateTime originalEnd;

    QDateTime sceneStartTime;
    double pixelsPerHour;

    bool temporary;
};

#endif
