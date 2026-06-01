#include "include/timelineview.h"
#include "include/graphicstaskitem.h"
#include "include/projectheaderitem.h"
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>

TimelineView::TimelineView(TaskBoardModel *model, QWidget *parent)
    : QWidget(parent), model(model), pixelsPerHour(50.0), tempTask(nullptr)
{
    sceneStartTime = QDateTime::currentDateTime();
    sceneStartTime.setTime(QTime(0,0,0));

    topScene = new QGraphicsScene(this);
    bottomScene = new QGraphicsScene(this);

    topView = new QGraphicsView(topScene);
    bottomView = new QGraphicsView(bottomScene);

    topView->setFixedHeight(40);
    topView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    topView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bottomView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    bottomView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(bottomView->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &TimelineView::syncHorizontalScrollbars);
    connect(topView->horizontalScrollBar(), &QScrollBar::valueChanged,
            bottomView->horizontalScrollBar(), &QScrollBar::setValue);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(topView);
    layout->addWidget(bottomView);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    QPushButton *addProjectBtn = new QPushButton("+ Добавить проект");
    layout->insertWidget(0, addProjectBtn);
    connect(addProjectBtn, &QPushButton::clicked, this, &TimelineView::onAddProject);

    connect(model, &TaskBoardModel::dataChanged, this, &TimelineView::refresh, Qt::QueuedConnection);
    refresh();
}

void TimelineView::syncHorizontalScrollbars(int value) {
    topView->horizontalScrollBar()->setValue(value);
}

void TimelineView::refresh() {
    deleteTempTask();

    for (auto items : taskItems) {
        for (auto item : items) {
            bottomScene->removeItem(item);
            delete item;
        }
    }
    taskItems.clear();
    for (auto header : projectHeaders) {
        bottomScene->removeItem(header);
        delete header;
    }
    projectHeaders.clear();

    topScene->clear();
    bottomScene->clear();

    double totalWidth = LEFT_MARGIN + 24 * pixelsPerHour + 50;
    double totalHeight = model->projectCount() * ROW_HEIGHT;
    bottomScene->setSceneRect(0, 0, totalWidth, totalHeight);
    topScene->setSceneRect(0, 0, totalWidth, 40);

    for (int hour = 0; hour <= 24; ++hour) {
        double x = LEFT_MARGIN + hour * pixelsPerHour;
        QDateTime dt = sceneStartTime.addSecs(hour * 3600);
        QGraphicsLineItem *line = topScene->addLine(x, 0, x, 20, QPen(Qt::gray));
        QGraphicsTextItem *text = topScene->addText(dt.toString("HH:00"));
        text->setPos(x + 2, 2);
        if (hour % 4 == 0)
            line->setPen(QPen(Qt::black, 2));
    }

    for (int p = 0; p < model->projectCount(); ++p) {
        double y =p * ROW_HEIGHT;
        bottomScene->addRect(0, y, totalWidth, ROW_HEIGHT-10, QPen(Qt::lightGray), QBrush(QColor(240, 240, 240)));

        ProjectHeaderItem *header = new ProjectHeaderItem(p, model);
        header->updatePosition(y);
        bottomScene->addItem(header);
        projectHeaders.append(header);
        bottomScene->addLine(LEFT_MARGIN, y + ROW_HEIGHT-10, totalWidth, y + ROW_HEIGHT-10, QPen(Qt::gray));

        // Задачи
        const QList<Task> &tasks = model->tasks(p);
        for (int t = 0; t < tasks.size(); ++t) {
            GraphicsTaskItem *item = new GraphicsTaskItem(tasks[t], p, t, model, nullptr, false);
            item->setViewParams(sceneStartTime, pixelsPerHour);
            item->updateGeometry(sceneStartTime, pixelsPerHour);
            item->setPos(LEFT_MARGIN, y);
            bottomScene->addItem(item);
            taskItems[p].append(item);
        }
    }
}

void TimelineView::onAddProject() {
    bool ok;
    QString name = QInputDialog::getText(this, "Новый проект", "Имя проекта:",
                                         QLineEdit::Normal, "Новый проект", &ok);
    if (ok && !name.isEmpty()) {
        model->addProject(name);
        QMetaObject::invokeMethod(this, [this]() {
            QScrollBar *vScroll = bottomView->verticalScrollBar();
            vScroll->setValue(vScroll->maximum());
        }, Qt::QueuedConnection);
    }
}

void TimelineView::mouseDoubleClickEvent(QMouseEvent *event) {
    QPoint viewPos = bottomView->mapFromGlobal(event->globalPos());
    if (!bottomView->rect().contains(viewPos)) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    QPointF scenePos = bottomView->mapToScene(viewPos);
    int projectIdx = scenePos.y() / ROW_HEIGHT;
    if (projectIdx < 0 || projectIdx >= model->projectCount()) return;
    QGraphicsItem *item = bottomScene->itemAt(scenePos, QTransform());
    if (item && dynamic_cast<GraphicsTaskItem*>(item)) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    double x = scenePos.x() - LEFT_MARGIN;
    if (x < 0) x = 0;
    if (x > 24 * pixelsPerHour) x = 24 * pixelsPerHour;
    qint64 hourOffset = qRound64(x / pixelsPerHour);
    if (hourOffset < 0) hourOffset = 0;
    if (hourOffset > 23) hourOffset = 23;
    QDateTime start = sceneStartTime.addSecs(hourOffset * 3600);
    QDateTime end = start.addSecs(3600);

    deleteTempTask();
    createTempTask(projectIdx, start, end);
}

void TimelineView::createTempTask(int projectIdx, const QDateTime &start, const QDateTime &end) {
    if (tempTask) return;
    Task tempTaskData;
    tempTaskData.name = "Новая задача";
    tempTaskData.start = start;
    tempTaskData.end = end;
    double y = projectIdx * ROW_HEIGHT;
    tempTask = new GraphicsTaskItem(tempTaskData, projectIdx, -1, model, this, true);
    tempTask->setViewParams(sceneStartTime, pixelsPerHour);
    tempTask->updateGeometry(sceneStartTime, pixelsPerHour);
    tempTask->setPos(LEFT_MARGIN, y);
    bottomScene->addItem(tempTask);
    tempTask->setFocus();
}

void TimelineView::commitTempTask(GraphicsTaskItem *item) {
    if (item && item == tempTask) {
        delete tempTask;
        tempTask = nullptr;
    }
}

void TimelineView::deleteTempTask() {
    if (tempTask) {
        delete tempTask;
        tempTask = nullptr;
    }
}
void TimelineView::cancelTempTask(GraphicsTaskItem *item) {
    if (item && item == tempTask) {
        delete tempTask;
        tempTask = nullptr;
    }
}
