#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPushButton>
#include "taskboard.h"
#include <QMouseEvent>
#include <QVBoxLayout>

class GraphicsTaskItem;
class ProjectHeaderItem;

struct TimeScaleStep {
    double intervalHours;
    int subDivisions;
    QString format;
    int minLabelSpacePx;
};

class TimelineView : public QWidget {
    Q_OBJECT
public:
    explicit TimelineView(TaskBoardModel *model, QWidget *parent = nullptr);

    void commitTempTask(GraphicsTaskItem *item);
    void cancelTempTask(GraphicsTaskItem *item);

    int getProjectIndexAtY(double sceneY) const;
    int getRowHeight() const { return ROW_HEIGHT; }
    int getLeftMargin() const { return LEFT_MARGIN; }

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void refresh();
    void onAddProject();
    void onSave();
    void onLoad();
    void syncHorizontalScrollbars(int value);

private:
    void setupToolbar();
    void createTempTask(int projectIdx, const QDateTime &start, const QDateTime &end);
    void deleteTempTask();
    void clearScenes();
    void setupSceneRects();
    void drawTopGrid();
    void drawBottomGrid();
    void drawProjectsAndTasks();
    QDateTime getTimeFromScenePos(const QPointF &scenePos) const;

    TimeScaleStep calculateTimeScaleStep(double pixelsPerHour) const;

    QGraphicsView *topView;
    QGraphicsView *bottomView;
    QGraphicsScene *topScene;
    QGraphicsScene *bottomScene;

    QVBoxLayout *mainLayout;
    TaskBoardModel *model;
    QDateTime sceneStartTime;
    double pixelsPerHour;

    QMap<int, QList<GraphicsTaskItem*>> taskItems;
    QList<ProjectHeaderItem*> projectHeaders;
    GraphicsTaskItem *tempTask;

    static const int LEFT_MARGIN = 150;
    static const int ROW_HEIGHT = 60;
    static const double MIN_PIXELS_PER_HOUR;
    static const double MAX_PIXELS_PER_HOUR;
};

#endif // TIMELINEVIEW_H
