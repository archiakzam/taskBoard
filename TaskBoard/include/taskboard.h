#ifndef TASKBOARD_H
#define TASKBOARD_H

#include <QObject>
#include <QDateTime>
#include <QList>
#include <QString>

struct Task {
    QString name;
    QDateTime start;
    QDateTime end;
};

class Project {
public:
    QString name;
    QList<Task> tasks;

    bool canAddTask(const Task &newTask) const;
    bool addTask(const Task &newTask);
    bool canResizeTask(int taskIdx, const QDateTime &newStart, const QDateTime &newEnd) const;
    bool resizeTask(int taskIdx, const QDateTime &newStart, const QDateTime &newEnd);
    void removeTask(int taskIdx);
    void renameTask(int taskIdx, const QString &newName);
};

class TaskBoardModel : public QObject {
    Q_OBJECT
    QList<Project> projects;
public:
    TaskBoardModel();

    int projectCount() const { return projects.size(); }
    QString projectName(int idx) const { return projects[idx].name; }
    const QList<Task>& tasks(int projectIdx) const { return projects[projectIdx].tasks; }

    void addProject(const QString &name);
    void renameProject(int idx, const QString &newName);
    void removeProject(int idx);

    bool addTask(int projectIdx, const Task &task);
    bool canResizeTask(int projectIdx, int taskIdx, const QDateTime &newStart, const QDateTime &newEnd) const;
    bool resizeTask(int projectIdx, int taskIdx, const QDateTime &newStart, const QDateTime &newEnd);
    void removeTask(int projectIdx, int taskIdx);
    void renameTask(int projectIdx, int taskIdx, const QString &newName);

    bool moveTask(int fromProjectIdx, int taskIdx, int toProjectIdx, const QDateTime &newStart, const QDateTime &newEnd);
    bool canAddTaskToProject(int projectIdx, const Task &task) const;

    void clearProjects();
signals:
    void dataChanged();
};

#endif // TASKBOARD_H
