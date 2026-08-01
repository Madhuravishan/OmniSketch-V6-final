#include "pen.h"

bool shouldStop(int currentDegree, int targetDegree, bool positive) {
    if (positive) {
        return currentDegree > targetDegree;
    } else {
        return currentDegree < targetDegree;
    }
}

void doSlowMove(Pen* pen, int startDegree, int targetDegree, int speedDegPerSec) {
    if (startDegree == targetDegree) {
        return;
    }

    auto startTime = millis();

    bool positive;
    if (targetDegree > startDegree) {
        positive = true;
    } else {
        positive = false;
    }

    auto currentDegree = startDegree;

    while (!(shouldStop(currentDegree, targetDegree, positive))) {
        pen->setRawValue(currentDegree);
        delay(10);

        auto currentTime = millis();
        auto deltaTime = currentTime - startTime;
        auto progressDegrees = int(double(deltaTime) / 1000 * speedDegPerSec);

        if (!positive) {
            progressDegrees = progressDegrees * -1;
        }

        currentDegree = startDegree + progressDegrees;
    }
    pen->setRawValue(targetDegree);
    delay(200);
}


// OMNISKETCH: servo pin and mirror flag are passed in.
//   Pen 1: Pen(SERVO_PEN1_PIN)          -> not mirrored, frame is 0-90 = physical
//   Pen 2: Pen(SERVO_PEN2_PIN, true)    -> mirrored, physical = 180 - logical
Pen::Pen(int servoPin, bool mirrored)
{
    this->servoPin = servoPin;
    this->mirrored = mirrored;
    servo = new Servo();
    servo->attach(servoPin);
    // Initial position: 90 (up). 90 is the symmetry axis of the mirror flip,
    // so this works correctly for both mirrored and non-mirrored pens.
    setRawValue(90);
}

// OMNISKETCH: The "raw value" name is retained for backward compatibility but
// the input is now treated as a LOGICAL angle in pen-1's frame (0-90). For
// a mirrored pen we flip it about 90 deg before writing to the servo, so
// the slider, park button, and slowUp/slowDown all work without knowing
// which pen they're driving.
void Pen::setRawValue(int rawValue) {
    int physicalAngle = mirrored ? (180 - rawValue) : rawValue;
    this->servo->write(physicalAngle);
    currentPosition = rawValue;   // stored in logical frame
}

void Pen::setPenDistance(int value) {
    Serial.println("Pen distance angle set to " + String(value));
    this->penDistance = value;
}

void Pen::slowUp() {
    if (penDistance == -1) {
        throw std::invalid_argument("not ready");
    }

    doSlowMove(this, currentPosition, 90, slowSpeedDegPerSec);
    currentPosition = 90;
}

void Pen::slowDown() {
    if (penDistance == -1) {
        throw std::invalid_argument("not ready");
    }

    doSlowMove(this, currentPosition, penDistance, slowSpeedDegPerSec);
    currentPosition = penDistance;
}

bool Pen::isDown() {
    return currentPosition == penDistance;
}

// OMNISKETCH: a pen is "ready" once its down-angle has been calibrated.
// Used by ToolChangeTask so an uncalibrated pen is never actuated.
bool Pen::isReady() {
    return penDistance != -1;
}

