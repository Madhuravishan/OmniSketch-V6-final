#ifndef Runner_h
#define Runner_h
#include "movement.h"
#include "tasks/task.h"
#include "pen.h"
#include "display.h"
#include "LittleFS.h"
class Runner {
    private:
    Movement *movement;
    Pen *penA;
    Pen *penB;
    Pen *activePen;
    Display *display;
    void initTaskProvider();
    Task* getNextTask();
    Task* currentTask;
    bool stopped;
    File openedFile;
    double totalDistance;
    double distanceSoFar;
    Movement::Point startPosition;
    Movement::Point targetPosition;
    int progress;
    Task *finishingSequence[1];
    int sequenceIx = 0;

    // OMNISKETCH (Step 4): bounding box of the active drawing, parsed from
    // the optional 'b' header line. Used by the Preview Boundary button.
    bool hasBoundingBox = false;
    double bboxMinX = 0, bboxMinY = 0, bboxMaxX = 0, bboxMaxY = 0;

    // OMNISKETCH (Step 4): when true, runner is doing a one-shot boundary
    // trace only - on completion it stops without continuing to drawing.
    bool previewOnly = false;

    // OMNISKETCH (Step 4 fix): the start beep MUST NOT run inside start()
    // because /run is dispatched on the AsyncTCP task. A blocking delay
    // there starves AsyncTCP and trips the task watchdog. Instead start()
    // raises this flag; run() consumes it on its first iteration (which
    // executes on the main loop task where blocking delays are safe).
    bool pendingStartBeep = false;

    public:
    Runner(Movement *movement, Pen *penA, Pen *penB, Display *display);
    void start();
    void run();
    void dryRun();
    void runBoundaryPreview();
};
#endif

