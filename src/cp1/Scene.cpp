// Scene.cpp - overview and focus views.
//
// Overview draw order (painter's algorithm):
//   background -> far halves of orbits/trails -> planets behind the Sun ->
//   Sun -> near halves of orbits/trails -> planets in front.
// Focus view: overview, dimmed -> focused body at its zoomed position -> panel.
#include "galaxis/Scene.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "galaxis/Components.h"
#include "galaxis/Facts.h"

namespace galaxis {

namespace {

const OrbitView kView{{640.0, 360.0}, 0.40};
constexpr int kSunRadius = 26;

// Focus layout: the body fills the left part of the screen, the panel sits right.
const Point kFocusCenter{370.0, 360.0};
constexpr int kPanelX = 740, kPanelY = 80, kPanelW = 480, kPanelH = 560;
const Vec3 kFocusLight{-0.80, -0.32, 0.50};  // Sun to the upper-left: clear day/night line on the right
constexpr double kFocusRingTilt = 0.30;

struct Body {
    int index;
    double angle;
    double rx;
    Point pos;
    bool nearSide;
};

std::vector<Body> layoutPlanets(const SceneState& s) {
    const auto& sys = solarSystem();
    std::vector<Body> bodies;
    for (int i = 0; i < static_cast<int>(sys.size()); ++i) {
        const PlanetInfo& p = sys[static_cast<std::size_t>(i)];
        const double a = planetAngle(p, s.day);
        const double rx = orbitRadiusPx(p.orbitAU);
        bodies.push_back({i, a, rx, orbitPoint(kView, rx, a), isNearSide(a)});
    }
    // Farther (higher on screen) first, so nearer planets overlap them.
    std::sort(bodies.begin(), bodies.end(), [](const Body& a, const Body& b) { return a.pos.y < b.pos.y; });
    return bodies;
}

void drawPlanetBody(Canvas& c, const PlanetInfo& p, Point pos, int r, Vec3 light, double ringTilt, double spin) {
    if (p.rings) drawRings(c, pos, r, ringTilt, Half::Top);
    drawAtmosphereHalo(c, p, pos, r, light);
    drawPlanet(c, p, pos, r, light, spin);
    if (p.rings) drawRings(c, pos, r, ringTilt, Half::Bottom);
}

void drawOverview(Canvas& c, const SceneState& s, const std::vector<Body>& bodies) {
    const auto& sys = solarSystem();
    drawBackground(c, 2026);

    auto orbitsAndTrails = [&](Half half) {
        if (!s.showOrbits) return;
        const double depthFade = (half == Half::Top) ? 0.7 : 1.0;  // far side a little dimmer
        for (const Body& b : bodies) {
            const PlanetInfo& p = sys[static_cast<std::size_t>(b.index)];
            drawOrbit(c, kView, b.rx, colors::Orbit, 0.38 * depthFade, half);
            drawTrail(c, kView, b.rx, b.angle, 0.6, mix(p.base, colors::White, 0.35), 0.6, half);
        }
    };
    auto planets = [&](bool nearSide) {
        for (const Body& b : bodies) {
            if (b.nearSide != nearSide || b.index == s.focus) continue;  // focused body is drawn by drawFocus
            const PlanetInfo& p = sys[static_cast<std::size_t>(b.index)];
            drawPlanetBody(c, p, b.pos, p.displayRadius, lightFromOrbit(b.angle, kView.tilt), kView.tilt, 0.0);
        }
    };

    orbitsAndTrails(Half::Top);
    planets(false);
    if (s.focus != kSunIndex) drawSun(c, kView.center, kSunRadius);
    orbitsAndTrails(Half::Bottom);
    planets(true);
}

// Darken the whole frame toward deep space so the focused body stands out.
void dim(Canvas& c, double amount) {
    if (amount <= 0) return;
    for (int y = 0; y < c.height(); ++y)
        for (int x = 0; x < c.width(); ++x) c.blendPixel(x, y, {2, 3, 9}, amount);
}

int focusRadius(int body) {
    if (body == kSunIndex) return 200;
    if (solarSystem()[static_cast<std::size_t>(body)].rings) return 140;  // leave room for the rings
    return 250;
}

void drawFocus(Canvas& c, const SceneState& s, const std::vector<Body>& bodies) {
    const double t = clamp01(s.zoom);
    const double e = easeInOutCubic(t);
    dim(c, 0.82 * e);

    // Where the body starts (its place in the overview) and where it ends.
    Point start = kView.center;
    double r0 = kSunRadius;
    Vec3 light0 = kFocusLight;
    double tilt0 = kView.tilt;
    if (s.focus != kSunIndex) {
        const Body& b = *std::find_if(bodies.begin(), bodies.end(), [&](const Body& x) { return x.index == s.focus; });
        start = b.pos;
        r0 = solarSystem()[static_cast<std::size_t>(s.focus)].displayRadius;
        light0 = lightFromOrbit(b.angle, kView.tilt);
    }
    const double r1 = focusRadius(s.focus);

    // The control point lifts the path up and over, like the camera gliding in.
    const Point control{0.5 * (start.x + kFocusCenter.x), std::min(start.y, kFocusCenter.y) - 140.0};
    const Point pos = bezier(start, control, kFocusCenter, e);
    const int r = static_cast<int>(std::lround(r0 * std::pow(r1 / r0, e)));  // geometric = feels like a zoom

    if (s.focus == kSunIndex) {
        drawSun(c, pos, r);
    } else {
        const PlanetInfo& p = solarSystem()[static_cast<std::size_t>(s.focus)];
        const Vec3 light = normalize({light0.x + (kFocusLight.x - light0.x) * e,
                                      light0.y + (kFocusLight.y - light0.y) * e,
                                      light0.z + (kFocusLight.z - light0.z) * e});
        const double tilt = tilt0 + (kFocusRingTilt - tilt0) * e;
        drawPlanetBody(c, p, pos, r, light, tilt, s.spin);
    }

    // Facts panel fades and slides in during the last part of the zoom.
    const double panelAlpha = smoothstep(0.55, 1.0, t);
    const int slide = static_cast<int>(std::lround((1.0 - panelAlpha) * 48));
    drawFactPanel(c, kPanelX + slide, kPanelY, kPanelW, kPanelH, factSheet(s.focus), panelAlpha);
}

}  // namespace

void renderScene(Canvas& c, const SceneState& s) {
    const std::vector<Body> bodies = layoutPlanets(s);
    drawOverview(c, s, bodies);
    if (s.focus >= 0 && s.focus < kBodyCount) drawFocus(c, s, bodies);
}

int pickBody(const SceneState& s, int x, int y) {
    const std::vector<Body> bodies = layoutPlanets(s);
    auto hit = [&](Point centre, double radius) {
        const double dx = x - centre.x, dy = y - centre.y;
        return dx * dx + dy * dy <= radius * radius;
    };
    auto hitRadius = [](int displayRadius) { return std::max(displayRadius + 4.0, 9.0); };  // small planets stay clickable

    // Reverse of the draw order: near-side planets (front-most first), Sun, far-side planets.
    for (auto it = bodies.rbegin(); it != bodies.rend(); ++it)
        if (it->nearSide && hit(it->pos, hitRadius(solarSystem()[static_cast<std::size_t>(it->index)].displayRadius)))
            return it->index;
    if (hit(kView.center, kSunRadius + 4.0)) return kSunIndex;
    for (auto it = bodies.rbegin(); it != bodies.rend(); ++it)
        if (!it->nearSide && hit(it->pos, hitRadius(solarSystem()[static_cast<std::size_t>(it->index)].displayRadius)))
            return it->index;
    return -1;
}

}  // namespace galaxis
