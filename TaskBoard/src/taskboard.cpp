#include "include/taskboard.h"

// ---------- Project ----------
bool Project::canAddTask(const Task &newTask) const {
    for (const Task &t : tasks) {
        if (!(newTask.end <= t.start || newTask.start >= t.end))
            return false;
    }
    return true;
}

bool Project::addTask(const Task &newTask) {
    if (!canAddTask(newTask)) return false;
    tasks.append(newTask);
    return true;
}

bool Project::canResizeTask(int taskIdx, const QDateTime &newStart, const QDateTime &newEnd) const {
    if (taskIdx < 0 || taskIdx >= tasks.size()) return false;
    for (int i = 0; i < tasks.size(); ++i) {
        if (i == taskIdx) continue;
        const Task &other = tasks[i];
        if (!(newEnd <= other.start || newStart >= other.end))
            return false;
    }
    return true;
}

bool Project::resizeTask(int taskIdx, const QDateTime &newStart, const QDateTime &newEnd) {
    if (!canResizeTask(taskIdx, newStart, newEnd)) return false;
    tasks[taskIdx].start = newStart;
    tasks[taskIdx].end = newEnd;
    return true;
}

void Project::removeTask(int taskIdx) {
    if (taskIdx >= 0 && taskIdx < tasks.size())
        tasks.removeAt(taskIdx);
}

void Project::renameTask(int taskIdx, const QString &newName) {
    if (taskIdx >= 0 && taskIdx < tasks.size())
        tasks[taskIdx].name = newName;
}

// ---------- TaskBoardModel ----------
TaskBoardModel::TaskBoardModel() {
}

void TaskBoardModel::addProject(const QString &name) {
    Project newProj;
    newProj.name = name;
    projects.append(newProj);
    emit dataChanged();
}

void TaskBoardModel::renameProject(int idx, const QString &newName) {
    if (idx < 0 || idx >= projects.size()) return;
    projects[idx].name = newName;
    emit dataChanged();
}

void TaskBoardModel::removeProject(int idx) {
    if (idx < 0 || idx >= projects.size()) return;
    projects.removeAt(idx);
    emit dataChanged();
}

bool TaskBoardModel::addTask(int projectIdx, const Task &task) {
    if (projectIdx < 0 || projectIdx >= projects.size()) return false;
    if (!projects[projectIdx].addTask(task)) return false;
    emit dataChanged();
    return true;
}

bool TaskBoardModel::canResizeTask(int projectIdx, int taskIdx,
                                   const QDateTime &newStart, const QDateTime &newEnd) const {
    if (projectIdx < 0 || projectIdx >= projects.size()) return false;
    return projects[projectIdx].canResizeTask(taskIdx, newStart, newEnd);
}

bool TaskBoardModel::resizeTask(int projectIdx, int taskIdx,
                                const QDateTime &newStart, const QDateTime &newEnd) {
    if (projectIdx < 0 || projectIdx >= projects.size()) return false;
    if (!projects[projectIdx].resizeTask(taskIdx, newStart, newEnd)) return false;
    emit dataChanged();
    return true;
}

void TaskBoardModel::removeTask(int projectIdx, int taskIdx) {
    if (projectIdx < 0 || projectIdx >= projects.size()) return;
    projects[projectIdx].removeTask(taskIdx);
    emit dataChanged();
}

void TaskBoardModel::renameTask(int projectIdx, int taskIdx, const QString &newName) {
    if (projectIdx < 0 || projectIdx >= projects.size()) return;
    projects[projectIdx].renameTask(taskIdx, newName);
    emit dataChanged();
}

bool TaskBoardModel::canAddTaskToProject(int projectIdx, const Task &task) const {
    if (projectIdx < 0 || projectIdx >= projects.size()) return false;
    return projects[projectIdx].canAddTask(task);
}

bool TaskBoardModel::moveTask(int fromProjectIdx, int taskIdx, int toProjectIdx,
                              const QDateTime &newStart, const QDateTime &newEnd) {
    if (fromProjectIdx < 0 || fromProjectIdx >= projects.size()) return false;
    if (toProjectIdx < 0 || toProjectIdx >= projects.size()) return false;
    if (taskIdx < 0 || taskIdx >= projects[fromProjectIdx].tasks.size()) return false;

    Task task = projects[fromProjectIdx].tasks[taskIdx];
    Task movedTask = task;
    movedTask.start = newStart;
    movedTask.end = newEnd;

    if (!projects[toProjectIdx].canAddTask(movedTask)) return false;

    projects[fromProjectIdx].removeTask(taskIdx);
    projects[toProjectIdx].addTask(movedTask);
    emit dataChanged();
    return true;
}

void TaskBoardModel::clearProjects() {
    projects.clear();
    emit dataChanged();
}
