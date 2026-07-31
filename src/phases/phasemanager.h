#ifndef PhaseManager_H
#define PhaseManager_H
#include "phase.h"
#include "movement.h"
#include "pen.h"
#include "runner.h"
#include <ESPAsyncWebServer.h>
class PhaseManager {
    private:
    Phase* currentPhase;
    Phase* retractBeltsPhase;
    Phase* setTopDistancePhase;
    Phase* extendToHomePhase;
    Phase* penCalibrationPhase;     // OMNISKETCH: pen 1 calibration
    Phase* penCalibration2Phase;    // OMNISKETCH (Step 3b): pen 2 calibration
    Phase* svgSelectPhase;
    Phase* beginDrawingPhase;
    Movement* movement;
    public:
    // OMNISKETCH (Step 3b): added PenCalibration2 between PenCalibration and SvgSelect.
    enum PhaseNames {RetractBelts, SetTopDistance, ExtendToHome, PenCalibration, PenCalibration2, SvgSelect, BeginDrawing};
    // OMNISKETCH (Step 3b): constructor now takes BOTH pens (penA = pen 1, penB = pen 2).
    PhaseManager(Movement* movement, Pen* penA, Pen* penB, Runner* runner, AsyncWebServer* server);
    Phase* getCurrentPhase();
    void setPhase(PhaseNames name);
    void respondWithState(AsyncWebServerRequest *request);
    void reset();
};
#endif

