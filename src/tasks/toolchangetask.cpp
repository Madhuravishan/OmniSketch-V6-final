#include "toolchangetask.h"

ToolChangeTask::ToolChangeTask(Pen *penA, Pen *penB) {
    this->penA = penA;
    this->penB = penB;
}

void ToolChangeTask::startRunning() {
    Serial.println("Tool change: raising both pens");
    // Only lift a pen that has been calibrated (penDistance set).
    // This keeps single-color runs and partially-calibrated test runs
    // from throwing on an unused pen.
    if (penA->isReady()) {
        penA->slowUp();
    }
    if (penB->isReady()) {
        penB->slowUp();
    }
    Serial.println("Tool change complete");
}

bool ToolChangeTask::isDone() {
    return true;
}
