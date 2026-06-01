#include "include/graphicstaskitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainterPath>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>

GraphicsTaskItem::GraphicsTaskItem(const Task &task, int projId, int taskId, TaskBoardModel *model,
                                   TimelineView *view, bool isTemporary, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), taskData(task), projectId(projId), taskId(taskId),
      model(model), timelineView(view), activeHandle(NoHandle), pixelsPerHour(50.0),
      temporary(isTemporary)
{
    if (temporary) {
        setBrush(QBrush(QColor(200, 200, 100, 150)));
        setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
    } else {
        setBrush(QBrush(QColor(135, 206, 250)));
        setPen(QPen(Qt::black, 1));
    }
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsFocusable);
}
void GraphicsTaskItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPointF pos = event->pos();
    ResizeHandle handle = handleAt(pos);
    if (handle != NoHandle) {
        setCursor(Qt::SizeHorCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsRectItem::hoverMoveEvent(event);
}
void GraphicsTaskItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setBrush(brush());
    painter->setPen(pen());
    painter->drawRect(rect());

    painter->setPen(Qt::black);
    QFont font = painter->font();
    font.setPointSize(10);
    painter->setFont(font);
    QRectF textRect = rect().adjusted(5, 5, -5, -5);
    QString elidedText = painter->fontMetrics().elidedText(taskData.name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
}

void GraphicsTaskItem::updateGeometry(const QDateTime &sceneStart, double pph) {
    prepareGeometryChange();
    sceneStartTime = sceneStart;
    pixelsPerHour = pph;
    qint64 startSec = sceneStartTime.secsTo(taskData.start);
    qint64 endSec = sceneStartTime.secsTo(taskData.end);
    double x = startSec / 3600.0 * pixelsPerHour;
    double width = (endSec - startSec) / 3600.0 * pixelsPerHour;
    setRect(x, 0, width, 50);
    update();
}

QPainterPath GraphicsTaskItem::shape() const {
    QPainterPath path;
    QRectF rect = this->rect();
    path.addRect(rect);
    int grabWidth = 10;
    QRectF leftHandle(rect.left(), rect.top(), grabWidth, rect.height());
    QRectF rightHandle(rect.right() - grabWidth, rect.top(), grabWidth, rect.height());
    path.addRect(leftHandle);
    path.addRect(rightHandle);
    return path;
}

GraphicsTaskItem::ResizeHandle GraphicsTaskItem::handleAt(const QPointF &pos) const {
    QRectF rect = this->rect();
    int grabWidth = 10;
    if (pos.x() <= rect.left() + grabWidth) return LeftHandle;
    if (pos.x() >= rect.right() - grabWidth) return RightHandle;
    return NoHandle;
}

void GraphicsTaskItem::setCursorForHandle(ResizeHandle handle) {
    switch (handle) {
    case LeftHandle:
    case RightHandle:
        setCursor(Qt::SizeHorCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
    }
}

bool GraphicsTaskItem::isWithinDayBoundary(const QDateTime &start, const QDateTime &end) const {
    QDateTime dayStart = sceneStartTime;
    QDateTime dayEnd = sceneStartTime.addSecs(24 * 3600);
    return (start >= dayStart && end <= dayEnd);
}

void GraphicsTaskItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    activeHandle = handleAt(pos);
    if (activeHandle != NoHandle) {
        setCursorForHandle(activeHandle);
        dragStartPos = pos;
        originalStart = taskData.start;
        originalEnd = taskData.end;
        event->accept();
    } else {
        QGraphicsRectItem::mousePressEvent(event);
    }
}

void GraphicsTaskItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (activeHandle == NoHandle) {
        QGraphicsRectItem::mouseMoveEvent(event);
        return;
    }
    QPointF delta = event->pos() - dragStartPos;
    double deltaHours = delta.x() / pixelsPerHour;
    QDateTime newStart = originalStart;
    QDateTime newEnd = originalEnd;

    if (activeHandle == LeftHandle) {
        newStart = originalStart.addSecs(qRound(deltaHours * 3600));
        if (!isWithinDayBoundary(newStart, newEnd)) return;
        if (newStart >= newEnd) return;
    } else if (activeHandle == RightHandle) {
        newEnd = originalEnd.addSecs(qRound(deltaHours * 3600));
        if (!isWithinDayBoundary(newStart, newEnd)) return;
        if (newEnd <= newStart) return;
    }

    if (temporary) {
        taskData.start = newStart;
        taskData.end = newEnd;
        updateGeometry(sceneStartTime, pixelsPerHour);
    } else {
        if (model->canResizeTask(projectId, taskId, newStart, newEnd)) {
            taskData.start = newStart;
            taskData.end = newEnd;
            updateGeometry(sceneStartTime, pixelsPerHour);
        }
    }
}

void GraphicsTaskItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (activeHandle != NoHandle) {
        if (taskData.start != originalStart || taskData.end != originalEnd) {
            if (!temporary) {
                if (!model->resizeTask(projectId, taskId, taskData.start, taskData.end)) {
                    taskData.start = originalStart;
                    taskData.end = originalEnd;
                    updateGeometry(sceneStartTime, pixelsPerHour);
                }
            }
        }
        activeHandle = NoHandle;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsRectItem::mouseReleaseEvent(event);
    }
}

void GraphicsTaskItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    if (temporary) {
        commitTask();
    } else {
        bool ok;
        QString newName = QInputDialog::getText(nullptr, "Переименовать задачу",
                                                "Новое имя:", QLineEdit::Normal,
                                                taskData.name, &ok);
        if (ok && !newName.isEmpty())
            model->renameTask(projectId, taskId, newName);
    }
}

void GraphicsTaskItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    if (temporary) {
        QMenu menu;
        QAction *cancel = menu.addAction("Отменить создание");
        if (menu.exec(event->screenPos()) == cancel) {
            if (timelineView)
                    timelineView->cancelTempTask(this);
        }
        return;
    }
    QMenu menu;
    QAction *rename = menu.addAction("Переименовать задачу");
    QAction *remove = menu.addAction("Удалить задачу");
    QAction *selected = menu.exec(event->screenPos());
    if (selected == rename) {
        bool ok;
        QString newName = QInputDialog::getText(nullptr, "Переименовать задачу",
                                                "Новое имя:", QLineEdit::Normal,
                                                taskData.name, &ok);
        if (ok && !newName.isEmpty())
            model->renameTask(projectId, taskId, newName);
    } else if (selected == remove) {
        QMetaObject::invokeMethod(model, [this]() {
            model->removeTask(projectId, taskId);
        }, Qt::QueuedConnection);
    }
}

void GraphicsTaskItem::keyPressEvent(QKeyEvent *event) {
    if (temporary && event->key() == Qt::Key_Return) {
        commitTask();
    } else if (temporary && event->key() == Qt::Key_Escape) {
        if (timelineView)
                timelineView->cancelTempTask(this);
    } else {
        QGraphicsRectItem::keyPressEvent(event);
    }
}

void GraphicsTaskItem::commitTask() {
    if (!temporary) return;
    if (model->addTask(projectId, taskData)) {
        if (timelineView)
            timelineView->commitTempTask(this);
    } else {
        QMessageBox::warning(nullptr, "Ошибка", "Задача пересекается с существующей!");
    }
}

void GraphicsTaskItem::setTemporary(bool temp) {
    temporary = temp;
    if (temporary) {
        setBrush(QBrush(QColor(200, 200, 100, 150)));
        setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
    } else {
        setBrush(QBrush(QColor(135, 206, 250)));
        setPen(QPen(Qt::black, 1));
    }
    update();
}
