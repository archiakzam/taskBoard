#include "include/taskboardserializer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

bool TaskBoardSerializer::saveToFile(const TaskBoardModel &model, const QString &fileName) {
    QJsonArray projectsArray;
    for (int p = 0; p < model.projectCount(); ++p) {
        QJsonObject projObj;
        projObj["name"] = model.projectName(p);
        QJsonArray tasksArray;
        for (const Task &task : model.tasks(p)) {
            QJsonObject taskObj;
            taskObj["name"] = task.name;
            taskObj["start"] = task.start.toString(Qt::ISODate);
            taskObj["end"] = task.end.toString(Qt::ISODate);
            tasksArray.append(taskObj);
        }
        projObj["tasks"] = tasksArray;
        projectsArray.append(projObj);
    }

    QJsonDocument doc(projectsArray);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(doc.toJson());
    return true;
}

bool TaskBoardSerializer::loadFromFile(TaskBoardModel &model, const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
        return false;

    QList<Project> newProjects;
    QJsonArray projectsArray = doc.array();
    for (const QJsonValue &projVal : projectsArray) {
        QJsonObject projObj = projVal.toObject();
        Project proj;
        proj.name = projObj["name"].toString();
        QJsonArray tasksArray = projObj["tasks"].toArray();
        for (const QJsonValue &taskVal : tasksArray) {
            QJsonObject taskObj = taskVal.toObject();
            Task task;
            task.name = taskObj["name"].toString();
            task.start = QDateTime::fromString(taskObj["start"].toString(), Qt::ISODate);
            task.end = QDateTime::fromString(taskObj["end"].toString(), Qt::ISODate);
            if (task.start.isValid() && task.end.isValid() && task.start <= task.end) {
                proj.tasks.append(task);
            }
        }
        newProjects.append(proj);
    }

    model.clearProjects();
    for (const Project &proj : newProjects) {
        model.addProject(proj.name);
        int lastIdx = model.projectCount() - 1;
        for (const Task &task : proj.tasks) {
            model.addTask(lastIdx, task);
        }
    }
    return true;
}
