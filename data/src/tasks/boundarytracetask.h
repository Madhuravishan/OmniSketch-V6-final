#ifndef BoundaryTraceTask_h
#define BoundaryTraceTask_h
#include "movement.h"
#include "task.h"

// OMNISKETCH (Step 4): Traces the drawing's bounding box with the laser on.
// Used only via the manual "Preview Boundary" button on the BeginDrawing
// slide. Both pens are assumed UP before this task runs; the task does
// not touch the pens.
class BoundaryTraceTask : public Task {
    private:
    static constexpr int NUM_LEGS = 5;        // 4 corners + return to start
    Movement *movement;
    Movement::Point corners[NUM_LEGS];
    int legIndex;
    bool laserOn;
    public:
    BoundaryTraceTask(Movement *movement, double minX, double minY, double maxX, double maxY);
    void startRunning();
    bool isDone();
    static constexpr const char* NAME = "BoundaryTraceTask";
    const char* name() {
        return NAME;
    }
};
#endif

