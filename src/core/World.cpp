// World.cpp - simulation, cameras, picking and the Bezier fly-to.
#include "galaxis/World.h"

#include <algorithm>
#include <cmath>

namespace galaxis {

namespace {

constexpr double kFlyInSeconds = 1.8;
constexpr double kFlyOutSeconds = 1.4;
const Vec3 kUp{0, 1, 0};

}  // namespace

// ================================================================ layout
double orbitRadiusWorld(double au) { return 3.0 + 7.0 * std::sqrt(au); }

double bodyRadiusWorld(int body) {
    if (body == kSunIndex) return 1.9;
    return solarSystem()[static_cast<std::size_t>(body)].displayRadius / 13.5;
}

double axialTiltRadians(int body) {
    // Obliquity in degrees (Venus' 177 degree flip is shown as retrograde spin instead).
    static const double kTilt[kBodyCount] = {0.03, 2.6, 23.4, 25.2, 3.1, 26.7, 97.8, 28.3, 7.25};
    return radians(kTilt[body]);
}

double spinPeriodSeconds(int body) {
    // Real rotation periods range from 10 hours to 243 days, far too wide to
    // show directly. The fourth root keeps the order (Jupiter fastest, Venus
    // slowest) but squeezes it into roughly 3 to 16 seconds per turn.
    const double hours = (body == kSunIndex) ? 27.0 * 24.0
                                             : std::abs(solarSystem()[static_cast<std::size_t>(body)].rotationHours);
    double period = 4.0 * std::pow(hours / 24.0, 0.25);
    if (body != kSunIndex && solarSystem()[static_cast<std::size_t>(body)].rotationHours < 0) period = -period;
    return period;
}

double orbitAngle(int body, double day) {
    const PlanetInfo& p = solarSystem()[static_cast<std::size_t>(body)];
    return p.phase0 - 2.0 * kPi * day / p.periodDays;
}

// ================================================================ camera
Vec3 OrbitCamera::eye() const {
    const double cp = std::cos(pitch);
    return target + Vec3{cp * std::sin(yaw), std::sin(pitch), cp * std::cos(yaw)} * distance;
}

void OrbitCamera::rotate(double dYaw, double dPitch) {
    yaw += dYaw;
    pitch = std::clamp(pitch + dPitch, radians(-80.0), radians(85.0));
}

void OrbitCamera::zoom(double factor) { distance = std::clamp(distance * factor, 8.0, 320.0); }

void OrbitCamera::pan(double dx, double dy) {
    const CameraPose p = pose();
    const double h = 2.0 * distance * std::tan(kFovY / 2);  // visible height at the target
    Vec3 right = p.right();
    Vec3 forwardFlat = normalize(Vec3{p.forward().x, 0, p.forward().z});
    target += right * (-dx * h) + forwardFlat * (dy * h);
    target.y = 0;  // stay on the orbital plane
    const double len = length(target);
    if (len > 60) target = target * (60 / len);
}

// ================================================================ app
void Galaxis::update(double dt) {
    if (!paused) {
        day += daysPerSecond * dt;
        clock += dt;
    }
    if (phase_ == Phase::FlyIn) {
        flightT_ += dt / kFlyInSeconds;
        if (flightT_ >= 1.0) phase_ = Phase::Focused;
    } else if (phase_ == Phase::FlyOut) {
        flightT_ += dt / kFlyOutSeconds;
        if (flightT_ >= 1.0) {
            phase_ = Phase::Overview;
            focus_ = -1;
        }
    }
}

void Galaxis::select(int b) {
    if (b < 0 || b >= kBodyCount) return;
    if (b == focus_ && (phase_ == Phase::FlyIn || phase_ == Phase::Focused)) return;
    flightFrom_ = pose();  // start from wherever the camera is right now
    focus_ = b;
    phase_ = Phase::FlyIn;
    flightT_ = 0.0;
}

void Galaxis::deselect() {
    if (phase_ != Phase::FlyIn && phase_ != Phase::Focused) return;
    flightFrom_ = pose();
    phase_ = Phase::FlyOut;
    flightT_ = 0.0;
}

double Galaxis::focusAmount() const {
    switch (phase_) {
        case Phase::Overview: return 0.0;
        case Phase::FlyIn: return easeInOutCubic(flightT_);
        case Phase::Focused: return 1.0;
        case Phase::FlyOut: return 1.0 - easeInOutCubic(flightT_);
    }
    return 0.0;
}

double Galaxis::panelAlpha() const {
    switch (phase_) {
        case Phase::Overview: return 0.0;
        case Phase::FlyIn: return smoothstep(0.6, 1.0, flightT_);
        case Phase::Focused: return 1.0;
        case Phase::FlyOut: return 1.0 - smoothstep(0.0, 0.25, flightT_);
    }
    return 0.0;
}

BodyState Galaxis::body(int i) const {
    BodyState b;
    b.radius = bodyRadiusWorld(i);
    if (i == kSunIndex) {
        b.position = {0, 0, 0};
    } else {
        const double a = orbitAngle(i, day);
        const double r = orbitRadiusWorld(solarSystem()[static_cast<std::size_t>(i)].orbitAU);
        b.position = {r * std::cos(a), 0, r * std::sin(a)};
    }
    // Composite transformation: spin about the planet's own axis, then tilt
    // that axis over (rotation about z).
    const double spin = 2.0 * kPi * clock / spinPeriodSeconds(i);
    const Mat4 tilt = rotateZ(-axialTiltRadians(i));
    b.orientation = tilt * rotateY(spin);
    b.axis = transformDirection(tilt, {0, 1, 0});
    return b;
}

CameraPose Galaxis::focusPose(int b) const {
    const BodyState s = body(b);
    const double R = s.radius;
    const double tanHalf = std::tan(kFovY / 2);

    // How much of the screen height the body's radius should fill.
    double frac = 0.36;
    if (b == kSunIndex) frac = 0.28;
    else if (solarSystem()[static_cast<std::size_t>(b)].rings) frac = 0.17;  // leave room for the rings
    const double D = R / (2.0 * frac * tanHalf);

    // Choose a viewing direction that shows a lit face with the day/night line
    // visible: about 50 degrees round from the direction of the Sun, slightly above.
    Vec3 toSun = (b == kSunIndex) ? normalize(camera.eye() - camera.target) : normalize(-s.position);
    toSun.y = 0;
    toSun = normalize(toSun);
    const double theta = (b == kSunIndex) ? 0.0 : radians(50.0);
    Vec3 best{};
    for (int sign = -1; sign <= 1; sign += 2) {
        Vec3 c = transformDirection(rotateY(sign * theta), toSun);
        c = normalize(c + kUp * std::tan(radians(12.0)));
        const Vec3 fwd = -c;
        const Vec3 right = normalize(cross(fwd, kUp));
        // Keep the Sun on the left so the terminator sits on the right, beside the panel.
        if (b == kSunIndex || dot(toSun, right) < 0) {
            best = c;
            break;
        }
    }
    const Vec3 eye = s.position + best * D;
    const Vec3 right = normalize(cross(-best, kUp));
    const double shift = -kFocusScreenX * D * tanHalf * aspect;  // move the body to the left of the screen
    return {eye, s.position + right * shift};
}

CameraPose Galaxis::pose() const {
    const CameraPose overview = camera.pose();
    switch (phase_) {
        case Phase::Overview: return overview;
        case Phase::Focused: return focusPose(focus_);
        case Phase::FlyIn:
        case Phase::FlyOut: {
            const bool in = phase_ == Phase::FlyIn;
            const CameraPose from = flightFrom_;
            const CameraPose to = in ? focusPose(focus_) : overview;
            const double t = easeInOutCubic(flightT_);
            // Cubic Bezier for the eye: rise up out of the start pose, then
            // glide in along the final viewing direction.
            const Vec3 span = to.eye - from.eye;
            const double L = length(span);
            const Vec3 arrive = normalize(to.eye - to.target);
            const Vec3 leave = normalize(from.eye - from.target);
            const Vec3 p1 = from.eye + span * 0.30 + kUp * (0.18 * L) + (in ? Vec3{} : leave * (0.15 * L));
            const Vec3 p2 = to.eye + arrive * (0.35 * L) + kUp * (0.10 * L);
            const Vec3 eye = bezier(from.eye, p1, p2, to.eye, t);
            // The look-at point leads the eye slightly so the camera turns first.
            const double tt = easeInOutCubic(std::min(1.0, flightT_ * 1.25));
            return {eye, lerp(from.target, to.target, tt)};
        }
    }
    return overview;
}

Ray Galaxis::rayThrough(double ndcX, double ndcY) const {
    const CameraPose p = pose();
    const double t = std::tan(kFovY / 2);
    const Vec3 dir = p.forward() + p.right() * (ndcX * t * aspect) + p.up() * (ndcY * t);
    return {p.eye, normalize(dir)};
}

int Galaxis::pick(const Ray& ray, double viewportHeight, double minPixels) const {
    int best = -1;
    double bestT = 1e300;
    const double worldPerPixelAt1 = 2.0 * std::tan(kFovY / 2) / viewportHeight;
    for (int i = 0; i < kBodyCount; ++i) {
        const BodyState s = body(i);
        const Vec3 oc = s.position - ray.origin;
        const double along = dot(oc, ray.dir);
        if (along <= kNear) continue;
        const double r = std::max(s.radius, minPixels * worldPerPixelAt1 * along);
        const double d2 = dot(oc, oc) - along * along;  // squared distance from ray to centre
        if (d2 > r * r) continue;
        const double t = along - std::sqrt(r * r - d2);
        if (t < bestT) {
            bestT = t;
            best = i;
        }
    }
    return best;
}

}  // namespace galaxis
