#include "runner.h"
#include "tasks/movementtask.h"
#include "tasks/interpolatingmovementtask.h"
#include "tasks/pentask.h"
#include "tasks/toolchangetask.h"
#include "tasks/boundarytracetask.h"
#include "pen.h"
#include "display.h"
#include "config.h"
#include "LittleFS.h"
using namespace std;

// SCRUBBY (Step 4 v4): buzzer helpers using LEDC channel 15 directly.
//
// Previously these used Arduino's tone()/noTone(). Both tone() and the
// ESP32Servo library allocate LEDC channels starting from channel 0.
// When tone() ran during drawing (start beep, tool change beep) it
// grabbed channel 0 - the same channel pen 1's servo was using - and
// the servo PWM was corrupted. Observed symptom: pens stopped going
// down during drawing despite working in calibration.
//
// The fix: bypass Arduino's tone() entirely and use LEDC directly on
// channel 15, which is in the LEDC "low-speed" group (channels 8-15).
// That group has a hardware timer fully separate from the "high-speed"
// group (channels 0-7) that ESP32Servo uses. They cannot interfere.
//
// Still synchronous (delay) - safe ONLY from the main loop task,
// NEVER from inside an AsyncTCP HTTP handler (would trip the watchdog).
static void scrubbyBeep(int ms) {
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    ledcWriteTone(BUZZER_LEDC_CHANNEL, 2000);   // 2 kHz square wave
    delay(ms);
    ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);      // mute
    ledcDetachPin(BUZZER_PIN);
    // Restore plain GPIO output at the configured idle level so the
    // pin doesn't float between beeps.
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_HIGH ? LOW : HIGH);
}

static void scrubbyDoubleBeep(int ms, int gapMs) {
    scrubbyBeep(ms);
    delay(gapMs);
    scrubbyBeep(ms);
}

Runner::Runner(Movement *movement, Pen *penA, Pen *penB, Display *display) {
    stopped = true;
    this->movement = movement;
    this->penA = penA;
    this->penB = penB;
    this->activePen = penA;
    this->display = display;
}

void Runner::initTaskProvider() {
    openedFile = LittleFS.open("/commands");
    if (!openedFile || !openedFile.available()) {
        Serial.println("Failed to open file");
        throw std::invalid_argument("No File");
    }

    auto line = openedFile.readStringUntil('\n');
    if (line.charAt(0) == 'd') {
        totalDistance = line.substring(1, line.length() - 1).toDouble();
    } else {
        Serial.println("Bad file - no distance");
        throw std::invalid_argument("bad file");
    }

    auto heightLine = openedFile.readStringUntil('\n');
    if (heightLine.charAt(0) == 'h') {
        auto height = heightLine.substring(1, heightLine.length() - 1).toDouble();
    } else {
        Serial.println("Bad file - no height");
        throw std::invalid_argument("bad file");
    }

    // SCRUBBY (Step 4): optional bounding-box header. b<minX> <minY> <maxX> <maxY>
    auto pos = openedFile.position();
    auto bLine = openedFile.readStringUntil('\n');
    if (bLine.charAt(0) == 'b') {
        auto rest = bLine.substring(1);
        int s1 = rest.indexOf(' ');
        int s2 = rest.indexOf(' ', s1 + 1);
        int s3 = rest.indexOf(' ', s2 + 1);
        if (s1 > 0 && s2 > s1 && s3 > s2) {
            bboxMinX = rest.substring(0, s1).toDouble();
            bboxMinY = rest.substring(s1 + 1, s2).toDouble();
            bboxMaxX = rest.substring(s2 + 1, s3).toDouble();
            bboxMaxY = rest.substring(s3 + 1).toDouble();
            hasBoundingBox = true;
            Serial.printf("Boundary box: (%.1f, %.1f) -> (%.1f, %.1f)\n",
                          bboxMinX, bboxMinY, bboxMaxX, bboxMaxY);
        } else {
            Serial.println("Malformed b line, ignoring");
            hasBoundingBox = false;
        }
    } else {
        openedFile.seek(pos);
        hasBoundingBox = false;
    }

    Serial.println("Total distance to travel: " + String(totalDistance));

    distanceSoFar = 0;
    progress = -1;
    startPosition = movement->getCoordinates();

    activePen = penA;
    movement->setPenDP(PEN1_D_P_MM);

    auto homeCoordinates = movement->getHomeCoordinates();
    finishingSequence[0] = new InterpolatingMovementTask(movement, homeCoordinates);
}

void Runner::start() {
    initTaskProvider();
    previewOnly = false;
    pendingStartBeep = true;        // run() will beep + update display
    currentTask = getNextTask();
    currentTask->startRunning();
    stopped = false;
}

void Runner::runBoundaryPreview() {
    initTaskProvider();
    if (!hasBoundingBox) {
        Serial.println("Preview requested but file has no boundary box");
        return;
    }
    previewOnly = true;
    display->displayText("Preview...");
    currentTask = new BoundaryTraceTask(movement, bboxMinX, bboxMinY, bboxMaxX, bboxMaxY);
    currentTask->startRunning();
    stopped = false;
}

Task *Runner::getNextTask()
{
    if (openedFile.available())
    {
        auto line = openedFile.readStringUntil('\n');
        if (line.charAt(0) == 'p')
        {
            if (line.charAt(1) == '1')
            {
                return new PenTask(false, activePen);
            }
            else
            {
                return new PenTask(true, activePen);
            }
        }
        else if (line.charAt(0) == 't')
        {
            if (line.charAt(1) == '1')
            {
                Serial.println("Tool change -> pen 2");
                display->displayText("Tool change");
                activePen = penB;
                movement->setPenDP(PEN2_D_P_MM);
            }
            else
            {
                Serial.println("Tool change -> pen 1");
                display->displayText("Tool change");
                activePen = penA;
                movement->setPenDP(PEN1_D_P_MM);
            }
            scrubbyDoubleBeep(BUZZER_TOOL_CHANGE_BEEP_MS, BUZZER_TOOL_CHANGE_GAP_MS);
            return new ToolChangeTask(penA, penB);
        }
        else
        {
            auto x = line.substring(0, line.indexOf(" ")).toDouble();
            auto y = line.substring(line.indexOf(" ") + 1).toDouble();
            targetPosition = Movement::Point(x, y);
            return new InterpolatingMovementTask(movement, targetPosition);
        }
    }
    else
    {
        if (sequenceIx < (end(finishingSequence) - begin(finishingSequence))) {
            auto currentIx = sequenceIx;
            sequenceIx = sequenceIx + 1;
            return finishingSequence[currentIx];
        } else {
            display->displayText("Finished!");
            scrubbyBeep(BUZZER_END_BEEP_MS);
            delay(200);
            ESP.restart();
            return NULL;
        }
    }
}

void Runner::run()
{
    if (stopped)
    {
        return;
    }

    if (pendingStartBeep) {
        pendingStartBeep = false;
        display->displayText("Drawing...");
        scrubbyBeep(BUZZER_START_BEEP_MS);
    }

    if (currentTask->isDone())
    {
        if (previewOnly) {
            Serial.println("Boundary preview complete");
            display->displayText("Preview done");
            delete currentTask;
            currentTask = NULL;
            previewOnly = false;
            stopped = true;
            if (openedFile) {
                openedFile.close();
            }
            return;
        }

        if (currentTask->name() == InterpolatingMovementTask::NAME) {
            auto distanceCovered = Movement::distanceBetweenPoints(startPosition, targetPosition);
            distanceSoFar += distanceCovered;
            startPosition = targetPosition;
            auto newProgress = int(floor(distanceSoFar / totalDistance * 100));
            if (newProgress > 100) {
                newProgress = 100;
            }
            // SCRUBBY (Step 4 fix): throttle OLED updates to every 5%
            // (plus the 100% finish) using displayProgress().
            if (progress != newProgress) {
                Serial.println("Progress: " + String(newProgress));
                int oldProgress = progress;
                progress = newProgress;
                bool isFivePercentStep = (newProgress / 5) != (oldProgress / 5);
                bool isFinish          = (newProgress == 100);
                if (isFivePercentStep || isFinish) {
                    display->displayProgress(progress);
                }
            }
        }
        delete currentTask;
        currentTask = getNextTask();
        if (currentTask != NULL)
        {
            currentTask->startRunning();
        }
        else
        {
            stopped = true;
        }
    }
}

void Runner::dryRun() {
    initTaskProvider();
    auto task = getNextTask();
    auto index = 1;
    while (task != NULL) {
        index = index + 1;
        delete task;
        task = getNextTask();
    }
    Serial.println("All done");
}
