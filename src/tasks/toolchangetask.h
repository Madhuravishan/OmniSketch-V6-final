#ifndef ToolChangeTask_h
#define ToolChangeTask_h
#include "pen.h"
#include "task.h"

// SCRUBBY (Step 2): a tool change in a dual-pen drawing.
// Safety behaviour: before switching active pen, raise BOTH pens so
// neither one drags across the wall during the swap. The Runner sets
// which pen becomes active; this task only handles the physical lift.
class ToolChangeTask : public Task {
    private:
    const char* NAME = "ToolChangeTask";
    Pen *penA;
    Pen *penB;
    public:
    ToolChangeTask(Pen *penA, Pen *penB);
    bool isDone();
    void startRunning();
    const char* name() {
        return NAME;
    }
};
#endif
