#ifndef PenCalibrationPhase_h
#define PenCalibrationPhase_h
#include "notsupportedphase.h"
#include "phasemanager.h"
#include "pen.h"

// OMNISKETCH (Step 3b): generic single-pen calibration phase.
// Instantiated twice by PhaseManager - once for pen 1, once for pen 2 -
// each pointing at the appropriate next phase.
class PenCalibrationPhase : public NotSupportedPhase {
    private:
    PhaseManager* manager;
    Pen* pen;
    PhaseManager::PhaseNames nextPhase;   // OMNISKETCH: which phase to enter when this one completes
    const char* phaseName;                // OMNISKETCH: name reported in /getState ("PenCalibration" or "PenCalibration2")
    public:
    // OMNISKETCH: constructor now takes which pen, what to do next, and what to call ourselves.
    PenCalibrationPhase(PhaseManager* manager, Pen* pen, PhaseManager::PhaseNames nextPhase, const char* phaseName);
    void setServo(AsyncWebServerRequest *request);
    void setPenDistance(AsyncWebServerRequest *request);
    void doneWithPhase(AsyncWebServerRequest *request);   // OMNISKETCH: "Skip" button
    const char* getName();
};
#endif

