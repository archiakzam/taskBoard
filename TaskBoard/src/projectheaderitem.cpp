#include "include/projectheaderitem.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>

ProjectHeaderItem::ProjectHeaderItem(int projectIdx, TaskBoardModel *model, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), projectId(projectIdx), model(model)
{
    setRect(0, 0, 150, 50);
    setBrush(QBrush(QColor(230, 230, 230)));
    setPen(QPen(Qt::lightGray));
    setFlag(QGraphicsItem::ItemIsSelectable);
}

void ProjectHeaderItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setBrush(brush());
    painter->setPen(pen());
    painter->drawRect(rect());

    painter->setPen(Qt::black);
    QFont font = painter->font();
    font.setPointSize(10);
    painter->setFont(font);

    QRectF textRect = rect().adjusted(5, 0, -5, 0);
    QString projectName = model->projectName(projectId);
    QString elidedText = painter->fontMetrics().elidedText(projectName, Qt::ElideRight, textRect.width());

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
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
