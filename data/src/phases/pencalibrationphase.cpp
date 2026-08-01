#include "pencalibrationphase.h"

PenCalibrationPhase::PenCalibrationPhase(PhaseManager* manager, Pen* pen, PhaseManager::PhaseNames nextPhase, const char* phaseName) {
    this->manager = manager;
    this->pen = pen;
    this->nextPhase = nextPhase;
    this->phaseName = phaseName;
}

void PenCalibrationPhase::setServo(AsyncWebServerRequest *request) {
    const AsyncWebParameter* p = request->getParam(0);
    int angle = p->value().toInt();
    pen->setRawValue(angle);
    request->send(200, "text/plain", "OK");
}

void PenCalibrationPhase::setPenDistance(AsyncWebServerRequest *request) {
    const AsyncWebParameter* p = request->getParam(0);
    int angle = p->value().toInt();
    pen->setPenDistance(angle);
    pen->slowUp();
    // OMNISKETCH (Step 3b): transition to whichever phase this instance was wired to.
    // For pen 1 -> PenCalibration2.  For pen 2 -> BeginDrawing.
    manager->setPhase(nextPhase);
    manager->respondWithState(request);
}

// OMNISKETCH (Step 3b): "Skip" button for pen 2 calibration. The user clicks this
// when only one pen is mounted. We move on without calling setPenDistance(),
// so this pen's penDistance stays -1 and isReady() returns false. The
// ToolChangeTask already checks isReady() before lifting, so single-color
// drawings continue to work fine. A dual-color drawing that tries to use
// this pen will throw "not ready" cleanly when its first p1 token arrives.
void PenCalibrationPhase::doneWithPhase(AsyncWebServerRequest *request) {
    Serial.println("PenCalibrationPhase: skipped");
    manager->setPhase(nextPhase);
    manager->respondWithState(request);
}

const char* PenCalibrationPhase::getName() {
    return phaseName;
}

