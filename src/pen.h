#ifndef Pen_h
#define Pen_h
#include <ESP32Servo.h>
#include "config.h"   // SCRUBBY: SERVO_PEN1_PIN / SERVO_PEN2_PIN

const int RETRACT_DISTANCE = 20;
class Pen {
    private:
    Servo *servo;
    int servoPin;            // SCRUBBY: which GPIO this pen's servo is on
    bool mirrored;           // SCRUBBY (Step 3b): true if servo is mounted as a
                             //   mirror of pen 1. setRawValue() flips the angle
                             //   about 90 deg so the rest of the firmware/UI can
                             //   keep working in pen-1's frame (0-90).
    int penDistance = -1;
    int slowSpeedDegPerSec = 90;
    int currentPosition = 90;
    public:
    // SCRUBBY: pin and mirror flag are now constructor arguments.
    // main.cpp creates two pens:
    //   pen1 = new Pen(SERVO_PEN1_PIN);          // D33, not mirrored
    //   pen2 = new Pen(SERVO_PEN2_PIN, true);    // D32, MIRRORED
    Pen(int servoPin = SERVO_PEN1_PIN, bool mirrored = false);
    void setRawValue(int rawValue);
    void setPenDistance(int value);
    void slowUp();
    void slowDown();
    bool isDown();
    bool isReady();          // SCRUBBY: true once penDistance is calibrated
};
#endif
