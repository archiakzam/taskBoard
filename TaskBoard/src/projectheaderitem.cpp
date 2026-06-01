#include "include/projectheaderitem.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>

ProjectHeaderItem::ProjectHeaderItem(int projectIdx, TaskBoardModel *model, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), projectId(projectIdx), model(model)
{
    setRect(0, 0, 150, 50);
    setBrush(QBrush(QColor(230, 230, 230)));
    setPen(QPen(Qt::lightGray));
    label = new QGraphicsTextItem(model->projectName(projectIdx), this);
    label->setPos(10, 15);
    setFlag(QGraphicsItem::ItemIsSelectable);
}

void ProjectHeaderItem::updatePosition(double y) {
    setPos(0, y);
}

void ProjectHeaderItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    QMenu menu;
    QAction *rename = menu.addAction("Переименовать проект");
    QAction *remove = menu.addAction("Удалить проект");
    QAction *selected = menu.exec(event->screenPos());
    if (selected == rename) {
        bool ok;
        QString newName = QInputDialog::getText(nullptr, "Переименовать проект",
                                                "Новое имя:", QLineEdit::Normal,
                                                model->projectName(projectId), &ok);
        if (ok && !newName.isEmpty())
            model->renameProject(projectId, newName);
    } else if (selected == remove) {
        if (QMessageBox::question(nullptr, "Удалить проект",
                                  "Удалить проект и все его задачи?",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            QMetaObject::invokeMethod(model, [this]() {
                model->removeProject(projectId);
            }, Qt::QueuedConnection);
        }
    }
}

void ProjectHeaderItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    bool ok;
    QString newName = QInputDialog::getText(nullptr, "Переименовать проект",
                                            "Новое имя:", QLineEdit::Normal,
                                            model->projectName(projectId), &ok);
    if (ok && !newName.isEmpty())
        model->renameProject(projectId, newName);
}
