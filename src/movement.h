#ifndef Movement_h
#define Movement_h

#include "AccelStepper.h"
#include "Arduino.h" 
#include "display.h"
#include "config.h"   // OMNISKETCH: central hardware pin map (see config.h)

// Motor driver parameters.
constexpr int printSpeedSteps = 1000;
constexpr int  moveSpeedSteps = 3000;
constexpr long INFINITE_STEPS = 999999999;
constexpr long acceleration = 999999999;  // Essentially infinite, causing instant stop / start
constexpr int stepsPerRotation = STEPS_PER_ROTATION; // OMNISKETCH: from config.h

// Geometry parameters:
// Effective diameter of the pulley+belts. Use EStep calibration to refine this value.
constexpr double diameter = 12.69;          // [mm]
const double circumference = diameter * PI; // [mm]
constexpr double midPulleyToWall = 41.0;    // (Height) distance from mid of pulley to wall [mm].
constexpr float homedStepOffsetMM = 40.0;   // Length of fully retracted belt hitting stop screw.
const int homedStepsOffset = int((homedStepOffsetMM / circumference) * stepsPerRotation);
constexpr double mass_bot = 1.17;   // Mass of the mural bot [kg].
constexpr double g_constant = 9.81; // Earth's gravitational acceleration constant [m/s^2].
constexpr double d_t = 76.027;      // [mm] Distance of tangent points, where belts touch the pulleys.

// OMNISKETCH (Step 3c): d_p is now a per-pen runtime value (see Movement::penDP
// member below) instead of a constexpr. Default values for each pen are in
// config.h (PEN1_D_P_MM / PEN2_D_P_MM). d_m (CoM offset) and d_t (tangent-
// point spacing) are physical properties of the bot itself and stay fixed.
constexpr double d_m = 10.0 + PEN1_D_P_MM;  // [mm] CoM distance from belt line.
                                            // Defined relative to pen 1's d_p
                                            // because that's where Mural
                                            // historically tuned it.

constexpr double belt_elongation_coefficient = 5e-5;
const int HOME_Y_OFFSET_MM = 350;

// Margins used for transformations of the coordinate systems:
constexpr double safeYFraction = 0.2;
constexpr double safeXFraction = 0.2;

class Movement{
private:
    int topDistance;
    double minSafeY;
    double minSafeXOffset;
    double width;
    volatile bool moving;
    bool homed;
    double X = -1;
    double Y = -1;
    bool startedHoming;
    AccelStepper *leftMotor;
    AccelStepper *rightMotor;
    Display *display;
    void setOrigin();

    // OMNISKETCH (Step 3c): d_p is now a runtime member, switched by the
    // Runner on every tool change. Defaults to pen 1's value at startup
    // so single-color drawings behave identically to before this change.
    double penDP = PEN1_D_P_MM;

    struct Lengths {
        int left;
        int right;
        Lengths(int left, int right) {
            this->left = left;
            this->right = right;
        }
        Lengths() {

        }
    };

    Lengths getBeltLengths(double x, double y);

    double gamma_last_position = 0.0;
    // OMNISKETCH (Step 3c): removed `const` qualifier from these helpers because
    // they read penDP which is now a non-const member. (Could mark penDP
    // mutable instead, but plain non-const is clearer.)
    inline void getLeftTangentPoint(const double frameX, const double frameY, const double gamma, double& x_PL, double& y_PL);
    inline void getRightTangentPoint(const double frameX, const double frameY, const double gamma, double& x_PR, double& y_PR);
    void getBeltAngles(const double frameX, const double frameY, const double gamma, double& phi_L, double& phi_R);
    void getBeltForces(const double phi_L, const double phi_R, double& F_L, double&F_R) const;
    double solveTorqueEquilibrium(const double phi_L, const double phi_R, const double F_L, const double F_R, const double gamma_start) const;
    double getDilationCorrectedBeltLength(double belt_length, double F_belt) const;
    
public:
    Movement(Display *display);
    struct Point {
        double x;
        double y;
        Point(double x, double y) {
            this->x = x;
            this->y = y;
        }
        Point() {
        }
    };

    static double distanceBetweenPoints(Point point1, Point point2) {
        return sqrt(pow(point2.x - point1.x, 2) + pow(point2.y - point1.y, 2));
    }

    bool isMoving();
    bool hasStartedHoming();
    double getWidth();
    Point getCoordinates();
    void setTopDistance(const int distance);
    void resumeTopDistance(const int distance);
    int getTopDistance();
    void leftStepper(const int dir);
    void rightStepper(const int dir);
    int extendToHome();
    void runSteppers();
    float beginLinearTravel(double x, double y, int speed);

    // OMNISKETCH (Step 3c): called by Runner on tool change. Pass PEN1_D_P_MM
    // or PEN2_D_P_MM from config.h. Updates the kinematic geometry so all
    // subsequent moves are computed with the correct pen-to-belt-line offset.
    void setPenDP(double newDP);

    void extend1000mm(); 

    Point getHomeCoordinates();
    void disableMotors();
};

#endif

