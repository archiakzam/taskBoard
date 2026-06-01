#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPushButton>
#include "taskboard.h"
#include <QMouseEvent>

class GraphicsTaskItem;
class ProjectHeaderItem;

class TimelineView : public QWidget {
    Q_OBJECT
public:
    explicit TimelineView(TaskBoardModel *model, QWidget *parent = nullptr);
    void commitTempTask(GraphicsTaskItem *item);
    void cancelTempTask(GraphicsTaskItem *item);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void refresh();
    void onAddProject();
    void syncHorizontalScrollbars(int value);

private:
    void createTempTask(int projectIdx, const QDateTime &start, const QDateTime &end);
    void deleteTempTask();
    QGraphicsView *topView;
    QGraphicsView *bottomView;
    QGraphicsScene *topScene;
    QGraphicsScene *bottomScene;

    TaskBoardModel *model;
    QDateTime sceneStartTime;
    double pixelsPerHour;

    QMap<int, QList<GraphicsTaskItem*>> taskItems;
    QList<ProjectHeaderItem*> projectHeaders;
    GraphicsTaskItem *tempTask;

    static const int LEFT_MARGIN = 150;
    static const int ROW_HEIGHT = 60;
};

#endif
