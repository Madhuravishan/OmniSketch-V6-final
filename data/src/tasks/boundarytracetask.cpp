#include "boundarytracetask.h"
#include "movement.h"
#include "config.h"

BoundaryTraceTask::BoundaryTraceTask(Movement *movement, double minX, double minY, double maxX, double maxY) {
    this->movement = movement;
    corners[0] = Movement::Point(minX, minY);
    corners[1] = Movement::Point(maxX, minY);
    corners[2] = Movement::Point(maxX, maxY);
    corners[3] = Movement::Point(minX, maxY);
    corners[4] = Movement::Point(minX, minY);   // close the loop
    legIndex = -1;
    laserOn = false;
}

void BoundaryTraceTask::startRunning() {
    Serial.println("Boundary trace: laser ON, starting first leg");
    digitalWrite(LASER_PIN, LASER_ACTIVE_HIGH ? HIGH : LOW);
    laserOn = true;
    legIndex = 0;
    movement->beginLinearTravel(corners[0].x, corners[0].y, moveSpeedSteps);
}

bool BoundaryTraceTask::isDone() {
    // Still travelling the current leg? Wait.
    if (movement->isMoving()) {
        return false;
    }

    // Current leg finished. Advance to the next leg, if any.
    legIndex++;
    if (legIndex < NUM_LEGS) {
        movement->beginLinearTravel(corners[legIndex].x, corners[legIndex].y, moveSpeedSteps);
        return false;
    }

    // All legs done - turn the laser off and report done.
    if (laserOn) {
        Serial.println("Boundary trace: complete, laser OFF");
        digitalWrite(LASER_PIN, LASER_ACTIVE_HIGH ? LOW : HIGH);
        laserOn = false;
    }
    return true;
}
