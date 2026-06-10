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
#include <QWheelEvent>
#include<QPointF>
#include <QResizeEvent>

const double TimelineView::MIN_PIXELS_PER_HOUR = 30.0;
const double TimelineView::MAX_PIXELS_PER_HOUR = 200.0;

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

TimeScaleStep TimelineView::calculateTimeScaleStep(double pph) const {
    double targetPx = 100.0;
    double rawHours = targetPx / pph;

    if (pph >= 80.0) {
        if (rawHours <= 0.25)  return {0.25, 2, "HH:mm", 60};
        if (rawHours <= 0.5)   return {0.5,  1, "HH:mm", 70};
        return {1.0, 3, "HH:mm", 90};
    }
    else if (pph >= 20.0) {
        if (rawHours <= 2.0)   return {2.0, 3, "H'h'", 80};
        if (rawHours <= 4.0)   return {4.0, 3, "H'h'", 90};
        return {6.0, 2, "H'h'", 100};
    }
    else if (pph >= 5.0) {
        if (rawHours <= 12.0)  return {12.0, 2, "H'h'", 100};
        return {24.0, 4, "dd MMM", 120};
    }
    else {
        if (rawHours <= 48.0)  return {48.0, 2, "dd MMM", 120};
        if (rawHours <= 168.0) return {168.0, 6, "dd MMM", 150};
        return {336.0, 7, "MMM dd", 200};
    }
}

void TimelineView::drawTopGrid() {
    TimeScaleStep step = calculateTimeScaleStep(pixelsPerHour);
    double totalHours = 24.0;
    double subStep = step.intervalHours / step.subDivisions;

    QPen mainPen(Qt::black, 2);
    QPen subPen(Qt::gray, 1);
    double lastLabelX = -1e9;

    for (double hour = 0.0; hour <= totalHours + 0.001; hour += step.intervalHours) {
        double x = LEFT_MARGIN + hour * pixelsPerHour;
        QDateTime dt = sceneStartTime.addSecs(qRound64(hour * 3600));

        topScene->addLine(x, 0, x, 40, mainPen);
        if (x - lastLabelX >= step.minLabelSpacePx) {
            QGraphicsTextItem *text = topScene->addText(dt.toString(step.format));
            text->setPos(x + 2, 2);
            lastLabelX = x;
        }

        if (step.subDivisions > 1 && hour + subStep <= totalHours) {
            for (int i = 1; i < step.subDivisions; ++i) {
                double subHour = hour + i * subStep;
                if (subHour > totalHours + 0.001) break;
                double xs = LEFT_MARGIN + subHour * pixelsPerHour;
                topScene->addLine(xs, 25, xs, 40, subPen);
            }
        }
    }
}

void TimelineView::drawBottomGrid() {
    TimeScaleStep step = calculateTimeScaleStep(pixelsPerHour);
    double totalHours = 24.0;
    double subStep = step.intervalHours / step.subDivisions;

    QPen mainPen(Qt::gray, 1.5);
    QPen subPen(Qt::lightGray, 1, Qt::DashLine);

    for (double hour = 0.0; hour <= totalHours + 0.001; hour += step.intervalHours) {
        double x = LEFT_MARGIN + hour * pixelsPerHour;
        bottomScene->addLine(x, 0, x, bottomScene->height(), mainPen);

        if (step.subDivisions > 1 && hour + subStep <= totalHours) {
            for (int i = 1; i < step.subDivisions; ++i) {
                double subHour = hour + i * subStep;
                if (subHour > totalHours + 0.001) break;
                double xs = LEFT_MARGIN + subHour * pixelsPerHour;
                bottomScene->addLine(xs, 0, xs, bottomScene->height(), subPen);
            }
        }
    }
}

void TimelineView::drawProjectsAndTasks() {
    double totalWidth = LEFT_MARGIN + 24 * pixelsPerHour;
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

void TimelineView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = (event->angleDelta().y() > 0) ? 1.2 : 1.0/1.2;
        double newValue = pixelsPerHour * factor;
        newValue = qBound(MIN_PIXELS_PER_HOUR, newValue, MAX_PIXELS_PER_HOUR);
        if (qFuzzyCompare(pixelsPerHour, newValue)) return;

        QPointF scenePos = bottomView->mapToScene(bottomView->mapFromGlobal(event->globalPos()));
        double hourUnderCursor = (scenePos.x() - LEFT_MARGIN) / pixelsPerHour;

        pixelsPerHour = newValue;
        refresh();

        double newX = hourUnderCursor * pixelsPerHour + LEFT_MARGIN;
        bottomView->horizontalScrollBar()->setValue(newX - bottomView->viewport()->width()/2);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void TimelineView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    int availableWidth = bottomView->viewport()->width() - LEFT_MARGIN;
    if (availableWidth <= 0) return;

    double desired = static_cast<double>(availableWidth) / 24.0;
    double newPixelsPerHour = qBound(MIN_PIXELS_PER_HOUR, desired, MAX_PIXELS_PER_HOUR);
    if (qFuzzyCompare(pixelsPerHour, newPixelsPerHour)) return;

    double centerHour = (bottomView->horizontalScrollBar()->value() + bottomView->viewport()->width()/2.0) / pixelsPerHour;

    pixelsPerHour = newPixelsPerHour;
    refresh();

    int newPos = centerHour * pixelsPerHour - bottomView->viewport()->width()/2;
    bottomView->horizontalScrollBar()->setValue(qMax(0, newPos));
}
