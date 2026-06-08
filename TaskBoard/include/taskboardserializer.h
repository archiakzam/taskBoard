#ifndef TASKBOARDSERIALIZER_H
#define TASKBOARDSERIALIZER_H

#include <QString>
#include "taskboard.h"

class TaskBoardSerializer {
public:
    static bool saveToFile(const TaskBoardModel &model, const QString &fileName);
    static bool loadFromFile(TaskBoardModel &model, const QString &fileName);
};

#endif // TASKBOARDSERIALIZER_H
