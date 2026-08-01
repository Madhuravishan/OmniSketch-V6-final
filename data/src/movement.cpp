#include "movement.h"
#include "display.h"
#include <stdexcept>

Movement::Movement(Display *display)
{
    this->display = display;
   
    leftMotor = new AccelStepper(AccelStepper::DRIVER, LEFT_STEP_PIN, LEFT_DIR_PIN);
    leftMotor->setEnablePin(LEFT_ENABLE_PIN);
    leftMotor->setMaxSpeed(moveSpeedSteps);
    leftMotor->setPinsInverted(true);
    leftMotor->disableOutputs();

    rightMotor = new AccelStepper(AccelStepper::DRIVER, RIGHT_STEP_PIN, RIGHT_DIR_PIN);
    rightMotor->setEnablePin(RIGHT_ENABLE_PIN);
    rightMotor->setMaxSpeed(moveSpeedSteps);
    rightMotor->disableOutputs();

    topDistance = -1;
   
    moving = false;
    homed = false;
    startedHoming = false;
    // OMNISKETCH (Step 3c): penDP is initialised to PEN1_D_P_MM in the header.
    // Runner will call setPenDP() on tool changes.
};

void Movement::setTopDistance(const int distance) {
    Serial.printf("Top distance set to %s\n", String(distance));
    topDistance = distance;

    minSafeY = safeYFraction * topDistance;
    minSafeXOffset = safeXFraction * topDistance;
    width = topDistance - 2 * minSafeXOffset;
};

void Movement::resumeTopDistance(int distance) {
    setTopDistance(distance);
    homed = true;

    const Point homeCoordinates = getHomeCoordinates();
    X = homeCoordinates.x;
    Y = homeCoordinates.y;

    const Lengths lengths = getBeltLengths(homeCoordinates.x, homeCoordinates.y);
    leftMotor->setCurrentPosition(lengths.left);
    rightMotor->setCurrentPosition(lengths.right);

    moving = false;
}

void Movement::setOrigin()
{
    leftMotor->setCurrentPosition(homedStepsOffset);
    rightMotor->setCurrentPosition(homedStepsOffset);
    homed = true;
};

void Movement::leftStepper(const int dir)
{
    if (dir > 0)
    {
        leftMotor->move(INFINITE_STEPS);
        leftMotor->setSpeed(printSpeedSteps);
    }
    else if (dir < 0)
    {
        leftMotor->move(-INFINITE_STEPS);
        leftMotor->setSpeed(printSpeedSteps);
    }
    else
    {
        leftMotor->setAcceleration(acceleration);
        leftMotor->stop();
    }

    moving = true;
};

void Movement::rightStepper(const int dir)
{
    if (dir > 0)
    {
        rightMotor->move(INFINITE_STEPS);
        rightMotor->setSpeed(printSpeedSteps);
    }
    else if (dir < 0)
    {
        rightMotor->move(-INFINITE_STEPS);
        rightMotor->setSpeed(printSpeedSteps);
    }
    else
    {
        rightMotor->setAcceleration(acceleration);
        rightMotor->stop();
    }

    moving = true;
};

Movement::Point Movement::getHomeCoordinates() {
    if (topDistance == -1) {
        return Point(0, 0);
    }

    return Point(width / 2, HOME_Y_OFFSET_MM);
}

int Movement::extendToHome()
{
    setOrigin();

    auto homeCoordinates = getHomeCoordinates();
    startedHoming = true;
    auto moveTime = beginLinearTravel(homeCoordinates.x, homeCoordinates.y, moveSpeedSteps);
    return int(ceil(moveTime));
};

void Movement::runSteppers()
{
    if (moving)
    {
        leftMotor->runSpeedToPosition();
        rightMotor->runSpeedToPosition();

        if (leftMotor->distanceToGo() == 0 && rightMotor->distanceToGo() == 0)
        {
            moving = false;
        }
    }
};

// OMNISKETCH (Step 3c): switch the kinematic pen offset for the active pen.
// Also reset gamma_last_position so the iterative solver doesn't start from
// pen 1's last tilt estimate when pen 2 has a very different geometry; lets
// the solver converge cleanly on the next move.
void Movement::setPenDP(double newDP) {
    Serial.printf("Switching kinematic d_p from %f to %f\n", penDP, newDP);
    penDP = newDP;
    gamma_last_position = 0.0;
}

inline void Movement::getLeftTangentPoint(const double frameX, const double frameY, const double gamma, double& x_PL, double& y_PL) {
    // OMNISKETCH (Step 3c): now reads penDP instead of constexpr d_p.
    const double s_L = d_t / 2.0;
    const double P_LX = s_L * cos(gamma) - penDP * sin(gamma);
    const double P_LY = s_L * sin(gamma) + penDP * cos(gamma);
    x_PL = frameX - P_LX;
    y_PL = frameY - P_LY;
}

inline void Movement::getRightTangentPoint(const double frameX, const double frameY, const double gamma, double& x_PR, double& y_PR) {
    // OMNISKETCH (Step 3c): now reads penDP instead of constexpr d_p.
    const double s_R = d_t / 2.0;
    const double P_RX = s_R * cos(gamma) + penDP * sin(gamma);
    const double P_RY = s_R * sin(gamma) - penDP * cos(gamma);
    x_PR = frameX + P_RX;
    y_PR = frameY + P_RY;
}

void Movement::getBeltAngles(const double frameX, const double frameY, const double gamma, double& phi_L, double& phi_R) {
    double x_PL;
    double y_PL;
    getLeftTangentPoint(frameX, frameY, gamma, x_PL, y_PL);
    phi_L = atan2(y_PL, x_PL);

    double x_PR;
    double y_PR;
    getRightTangentPoint(frameX, frameY, gamma, x_PR, y_PR);
    phi_R = atan2(y_PR, topDistance - x_PR);
}

void Movement::getBeltForces(const double phi_L, const double phi_R, double& F_L, double&F_R) const {
    const double F_G = mass_bot * g_constant;
    F_R = F_G * cos(phi_L) / sin(phi_L + phi_R);
    F_L = F_G * cos(phi_R) / sin(phi_L + phi_R);
}

double Movement::solveTorqueEquilibrium(const double phi_L, const double phi_R, const double F_L, const double F_R, const double gamma_init) const {
    // NOTE: solver uses d_m (CoM offset, fixed) and d_t (tangent spacing, fixed).
    // It does NOT use d_p directly - the pen-specific geometry has already been
    // baked into phi_L / phi_R via getBeltAngles -> getTangentPoint -> penDP.
    const double s_L = d_t / 2.0;
    const double s_R = d_t / 2.0;

    double gamma_best = 99999999;
    double T_delta_best = 99999999;

    constexpr double gamma_step = 0.20 * PI / 180.0;
    constexpr double gamma_min = -90.0 * PI / 180.0;
    constexpr double gamma_max = 90.0 * PI / 180.0;
    constexpr double gamma_search_window = 2.0 * PI / 180.0;
    
    for (double gamma = gamma_init - gamma_search_window;
            gamma > gamma_min &&
            gamma < gamma_max &&
            gamma <= gamma_init + gamma_search_window;
            gamma += gamma_step){
        const double alpha = phi_L - gamma;
        const double beta = phi_R + gamma;
    
        double T_L = s_L * sin(alpha) * F_L;
        double T_R = s_R * sin(beta) * F_R;

        double s_m = d_m * tan(gamma);
        const double F_G = mass_bot * g_constant;
        double F_m = F_G * cos(gamma);
        double T_m = s_m * F_m;

        double T_delta = T_R - T_L + T_m;
        if (abs(T_delta) < abs(T_delta_best)){
            T_delta_best = T_delta;
            gamma_best = gamma;
        } else {
            return gamma_best;
        }
    }

    return gamma_best;
}

inline double Movement::getDilationCorrectedBeltLength(double belt_length, double F_belt) const {
    const double elongation_factor = 1 + belt_elongation_coefficient * F_belt;
    const double belth_length_corrected = belt_length / elongation_factor;
    return belth_length_corrected;
}

Movement::Lengths Movement::getBeltLengths(const double x, const double y) {
    const double frameX = x + minSafeXOffset;
    const double frameY = y + minSafeY;

    double gamma = gamma_last_position;
    double phi_L = 0.0;
    double phi_R = 0.0;
    double F_L = 0.0;
    double F_R = 0.0;
    constexpr int solver_max_iterations = 20;
    constexpr double gamma_delta_termination = 0.25 / 180.0 * PI;

    int debug_step_count = 0;
    for (int i = 0; i < solver_max_iterations; i++){
        getBeltAngles(frameX, frameY, gamma, phi_L, phi_R);

        getBeltForces(phi_L, phi_R, F_L, F_R);

        const double gamma_last = gamma;
        gamma = solveTorqueEquilibrium(phi_L, phi_R, F_L, F_R, gamma);
        debug_step_count = i;
        if (abs(gamma_last - gamma) < gamma_delta_termination) break;
    }
    gamma_last_position = gamma;

    double leftX, leftY;
    double rightX, rightY;
    getLeftTangentPoint(frameX, frameY, gamma, leftX, leftY);
    getRightTangentPoint(frameX, frameY, gamma, rightX, rightY);

    const double leftLegFlat = sqrt(pow(leftX, 2) + pow(leftY, 2));
    const double rightLegFlat = sqrt(pow(topDistance - rightX, 2) + pow(rightY, 2));

    double leftLeg = sqrt(pow(leftLegFlat, 2) + pow(midPulleyToWall, 2));
    double rightLeg = sqrt(pow(rightLegFlat, 2) + pow(midPulleyToWall, 2));

    leftLeg = getDilationCorrectedBeltLength(leftLeg, F_L);
    rightLeg = getDilationCorrectedBeltLength(rightLeg, F_R);
    
    const double leftLegSteps = int((leftLeg / circumference) * stepsPerRotation);
    const double rightLegSteps = int((rightLeg / circumference) * stepsPerRotation);

    return Lengths(leftLegSteps, rightLegSteps);
}

float Movement::beginLinearTravel(double x, double y, int speed)
{
    X = x;
    Y = y;
    if (topDistance == -1 || !homed) {
        Serial.println("Not ready");
        throw std::invalid_argument("not ready");
    }

    if (x < 0 || (x - 1) > width)
    {
        Serial.println("Invalid x");
        throw std::invalid_argument("Invalid x");
    }

    if (y < 0)
    {
        Serial.println("Invalid y");
        throw std::invalid_argument("Invalid y");
    }

    auto lengths = getBeltLengths(x, y);
    auto leftLegSteps = lengths.left;
    auto rightLegSteps = lengths.right;

    auto deltaLeft = int(abs(abs(leftMotor->currentPosition()) - leftLegSteps));
    auto deltaRight = int(abs(abs(rightMotor->currentPosition()) - rightLegSteps));

    float leftSpeed, rightSpeed, moveTime;
    if (deltaLeft >= deltaRight)
    {
        leftSpeed = speed;
        moveTime = deltaLeft / leftSpeed;
        rightSpeed = deltaRight / moveTime;
    }
    else
    {
        rightSpeed = speed;
        moveTime = deltaRight / rightSpeed;
        leftSpeed = deltaLeft / moveTime;
    }

    leftMotor->moveTo(leftLegSteps);
    leftMotor->setSpeed(leftSpeed);
    
    rightMotor->moveTo(rightLegSteps);
    rightMotor->setSpeed(rightSpeed);

    moving = true;
    return moveTime;
};

double Movement::getWidth() {
    if (topDistance == -1) {
        throw std::invalid_argument("not ready");
    }
    return width;
}

Movement::Point Movement::getCoordinates() {
    if (X == -1 || Y == -1) {
        Serial.println("Not ready to get coordinates");
        throw std::invalid_argument("not ready");
    }

    if (moving) {
        Serial.println("Can't get coordinates while moving");
        throw std::invalid_argument("not ready");
    }
    return Movement::Point(X, Y);
}

void Movement::extend1000mm() {
    const int steps = int((1000 / circumference) * stepsPerRotation);   

    leftMotor->move(steps);
    leftMotor->setSpeed(moveSpeedSteps);

    rightMotor->move(steps);
    rightMotor->setSpeed(moveSpeedSteps);

    moving = true;
}

void Movement::disableMotors() {
    leftMotor->disableOutputs();
    rightMotor->disableOutputs();
}

bool Movement::isMoving() {
    return moving;
}

bool Movement::hasStartedHoming() {
    return startedHoming;
}

int Movement::getTopDistance() {
    return topDistance;
}

