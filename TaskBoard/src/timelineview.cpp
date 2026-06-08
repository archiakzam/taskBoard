#include "include/timelineview.h"
#include "include/graphicstaskitem.h"
#include "include/projectheaderitem.h"
#include "include/taskboardserializer.h"
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QFileDialog>

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

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(topView);
    mainLayout->addWidget(bottomView);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    setupToolbar();

    connect(model, &TaskBoardModel::dataChanged, this, &TimelineView::refresh, Qt::QueuedConnection);
    refresh();
}
void TimelineView::setupToolbar() {
    QPushButton *addProjectBtn = new QPushButton("+ Добавить проект");
    QPushButton *saveBtn = new QPushButton("Сохранить");
    QPushButton *loadBtn = new QPushButton("Загрузить");
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(addProjectBtn);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(loadBtn);
    buttonLayout->addStretch();
    mainLayout->insertLayout(0, buttonLayout);

    connect(addProjectBtn, &QPushButton::clicked, this, &TimelineView::onAddProject);
    connect(saveBtn, &QPushButton::clicked, this, &TimelineView::onSave);
    connect(loadBtn, &QPushButton::clicked, this, &TimelineView::onLoad);
}

void TimelineView::syncHorizontalScrollbars(int value) {
    topView->horizontalScrollBar()->setValue(value);
}

void TimelineView::refresh() {
    deleteTempTask();
    clearScenes();
    setupSceneRects();
    drawTopGrid();
    drawBottomGrid();
    drawProjectsAndTasks();
}

void TimelineView::clearScenes() {
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
}

void TimelineView::setupSceneRects() {
    double totalWidth = LEFT_MARGIN + 24 * pixelsPerHour + 50;
    double totalHeight = model->projectCount() * ROW_HEIGHT;
    bottomScene->setSceneRect(0, 0, totalWidth, totalHeight);
    topScene->setSceneRect(0, 0, totalWidth, 40);
}

void TimelineView::drawTopGrid() {
    double pixelsPerQuarter = pixelsPerHour / 4.0;
    QPen hourPen(Qt::black, 2);
    QPen minutePen(Qt::gray, 1);
    for (int hour = 0; hour <= 24; ++hour) {
        double x = LEFT_MARGIN + hour * pixelsPerHour;
        QDateTime dt = sceneStartTime.addSecs(hour * 3600);
        topScene->addLine(x, 0, x, 40, hourPen);
        QGraphicsTextItem *text = topScene->addText(dt.toString("HH:00"));
        text->setPos(x + 2, 2);
        if (hour < 24) {
            for (int q = 1; q <= 3; ++q) {
                double xm = x + q * pixelsPerQuarter;
                topScene->addLine(xm, 25, xm, 40, minutePen);
            }
        }
    }
}

void TimelineView::drawBottomGrid() {
    double pixelsPerQuarter = pixelsPerHour / 4.0;
    QPen hourPen(Qt::gray, 1.5);
    QPen minutePen(Qt::lightGray, 1, Qt::DashLine);
    for (int hour = 0; hour <= 24; ++hour) {
        double x = LEFT_MARGIN + hour * pixelsPerHour;
        bottomScene->addLine(x, 0, x, bottomScene->height(), hourPen);
        if (hour < 24) {
            for (int q = 1; q <= 3; ++q) {
                double xm = x + q * pixelsPerQuarter;
                bottomScene->addLine(xm, 0, xm, bottomScene->height(), minutePen);
            }
        }
    }
}

void TimelineView::drawProjectsAndTasks() {
    double totalWidth = LEFT_MARGIN + 24 * pixelsPerHour + 50;
    for (int p = 0; p < model->projectCount(); ++p) {
        double y = p * ROW_HEIGHT;
        bottomScene->addRect(0, y, totalWidth, ROW_HEIGHT-10, QPen(Qt::lightGray), QBrush(QColor(240, 240, 240)));

        ProjectHeaderItem *header = new ProjectHeaderItem(p, model);
        header->updatePosition(y);
        bottomScene->addItem(header);
        projectHeaders.append(header);
        bottomScene->addLine(LEFT_MARGIN, y + ROW_HEIGHT-10, totalWidth, y + ROW_HEIGHT-10, QPen(Qt::gray));

        const QList<Task> &tasks = model->tasks(p);
        for (int t = 0; t < tasks.size(); ++t) {
            GraphicsTaskItem *item = new GraphicsTaskItem(tasks[t], p, t, model, this, false);
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
    QDateTime start = getTimeFromScenePos(scenePos);
    QDateTime end = start.addSecs(3600);
    deleteTempTask();
    createTempTask(projectIdx, start, end);
}
QDateTime TimelineView::getTimeFromScenePos(const QPointF &scenePos) const {
    double x = scenePos.x() - LEFT_MARGIN;
    x = qBound(0.0, x, 24.0 * pixelsPerHour);
    qint64 hourOffset = qRound64(x / pixelsPerHour);
    hourOffset = qBound(0LL, hourOffset, 23LL);
    return sceneStartTime.addSecs(hourOffset * 3600);
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

int TimelineView::getProjectIndexAtY(double sceneY) const {
    int idx = static_cast<int>(sceneY / ROW_HEIGHT);
    if (idx >= 0 && idx < model->projectCount()) return idx;
    return -1;
}
void TimelineView::onSave() {
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить таск-борд", "",
                                                    "JSON files (*.json);;All files (*)");
    if (fileName.isEmpty()) return;
    if (!TaskBoardSerializer::saveToFile(*model, fileName))
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл!");
    else
        QMessageBox::information(this, "Успех", "Таск-борд сохранён.");
}

void TimelineView::onLoad() {
    QString fileName = QFileDialog::getOpenFileName(this, "Загрузить таск-борд", "",
                                                    "JSON files (*.json);;All files (*)");
    if (fileName.isEmpty()) return;
    if (!TaskBoardSerializer::loadFromFile(*model, fileName))
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить файл!");
    else
        QMessageBox::information(this, "Успех", "Таск-борд загружен.");
}
