#include <QApplication>
#include "include/timelineview.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TaskBoardModel model;
    TimelineView view(&model);
    view.setWindowTitle("Task Board");
    view.resize(1200, 600);
    view.show();
    return app.exec();
}
