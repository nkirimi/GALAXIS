// World.h - the Galaxis simulation, shared by CP2 (software renderer) and
// CP3 (OpenGL). Pure C++, no graphics code.
//
// World space: y is up, the planets orbit in the x-z plane, the Sun is at
// the origin. Units are arbitrary "scene units" (Neptune's orbit ~ 41).
//
// Galaxis holds everything that changes over time: simulation day, spin,
// the user's orbit camera, and the click-to-learn fly-to state machine
// (Overview -> FlyIn -> Focused -> FlyOut -> Overview).
#pragma once

#include "galaxis/Facts.h"
#include "galaxis/Math.h"
#include "galaxis/Planet.h"
#include "galaxis/Transform.h"

namespace galaxis {

constexpr double kFovY = 30.0 * kPi / 180.0;
constexpr double kNear = 0.05;
constexpr double kFar = 600.0;

// ------------------------------------------------------------ layout
double orbitRadiusWorld(double au);  // compressed (square-root) distance scale
double bodyRadiusWorld(int body);    // exaggerated sizes, body 8 = the Sun
double axialTiltRadians(int body);
double spinPeriodSeconds(int body);  // on-screen spin period (real rates scaled for visibility)
double orbitAngle(int body, double day);

struct BodyState {
    Vec3 position;
    double radius;
    Mat4 orientation;  // axial tilt * spin: object space -> world space (rotation only)
    Vec3 axis;         // spin axis in world space
};

// ------------------------------------------------------------ cameras
struct CameraPose {
    Vec3 eye, target;
    Mat4 view() const { return lookAt(eye, target, {0, 1, 0}); }
    Vec3 forward() const { return normalize(target - eye); }
    Vec3 right() const { return normalize(cross(forward(), {0, 1, 0})); }
    Vec3 up() const { return cross(right(), forward()); }
};

// The user-controlled camera: it orbits a target point.
struct OrbitCamera {
    Vec3 target{0, 0, 0};
    double yaw = 0.0;               // around the y axis
    double pitch = radians(24.0);   // above the orbital plane
    double distance = 100.0;

    Vec3 eye() const;
    CameraPose pose() const { return {eye(), target}; }
    void rotate(double dYaw, double dPitch);
    void zoom(double factor);
    void pan(double dx, double dy);  // in units of the current view width/height
};

struct Ray {
    Vec3 origin, dir;
};

// ------------------------------------------------------------ the app
class Galaxis {
public:
    enum class Phase { Overview, FlyIn, Focused, FlyOut };

    // Time
    double day = 0.0;
    double daysPerSecond = 20.0;
    bool paused = false;
    double clock = 0.0;  // seconds of animation time (drives spin)

    bool showOrbits = true;
    double aspect = 16.0 / 9.0;
    OrbitCamera camera;

    void update(double dt);

    // Click-to-learn
    void select(int body);  // fly the camera to a body (0..7 planets, 8 Sun)
    void deselect();        // fly back to the overview
    int focus() const { return focus_; }
    Phase phase() const { return phase_; }
    double focusAmount() const;  // 0 = overview, 1 = fully focused (for dimming)
    double panelAlpha() const;   // fade of the facts panel

    // Camera for the current frame
    CameraPose pose() const;
    Mat4 view() const { return pose().view(); }
    Mat4 projection() const { return perspective(kFovY, aspect, kNear, kFar); }

    BodyState body(int i) const;

    // Picking: ray from the eye through a point given in NDC (-1..1, y up).
    Ray rayThrough(double ndcX, double ndcY) const;
    // Front-most body hit by the ray, or -1. Small bodies get a minimum
    // on-screen pick radius of `minPixels` for a viewport `viewportHeight` tall.
    int pick(const Ray& ray, double viewportHeight, double minPixels = 10.0) const;

    // Where the focused body sits on screen when fully zoomed in.
    static constexpr double kFocusScreenX = -0.42;  // NDC x of the body centre (left side)

private:
    CameraPose focusPose(int body) const;

    int focus_ = -1;
    Phase phase_ = Phase::Overview;
    double flightT_ = 0.0;
    CameraPose flightFrom_{};
};

}  // namespace galaxis
