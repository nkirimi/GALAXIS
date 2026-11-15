// Components.h - the simplified graphical building blocks of Galaxis,
// each built only from the algorithms in Raster.h / Clip.h / Font.h.
//
// Every component takes its position, size and angles as parameters, so
// CP2 (transformations + animation) can drive them frame by frame without
// changing their drawing code.
#pragma once

#include <string>

#include "galaxis/Canvas.h"
#include "galaxis/Clip.h"
#include "galaxis/Facts.h"
#include "galaxis/Math.h"
#include "galaxis/Planet.h"
#include "galaxis/Raster.h"

namespace galaxis {

// ------------------------------------------------------------- Orbit view
// The orbital plane is seen from a raised viewpoint, so a circular orbit of
// radius rx appears as an ellipse with ry = rx * tilt (tilt = sin of the
// viewing elevation). Angle 0 is to the right of the Sun; positive sin(angle)
// is the near side (lower on screen, in front of the Sun).
struct OrbitView {
    Point center;   // screen position of the Sun
    double tilt;    // ry / rx
};

double orbitRadiusPx(double au);  // compressed (square-root) distance scale
Point orbitPoint(const OrbitView& v, double rx, double angle);
bool isNearSide(double angle);

// Direction of sunlight at a planet, in screen space (x right, y down,
// z toward the viewer), for a planet at `angle` on its orbit.
Vec3 lightFromOrbit(double angle, double tilt);

// ------------------------------------------------------------ Components
void drawBackground(Canvas& c, unsigned seed);  // gradient + nebula + starfield
void drawSun(Canvas& c, Point center, int radius, unsigned seed = 7);

// `rotation` (radians) spins the surface texture about the planet's axis.
void drawPlanet(Canvas& c, const PlanetInfo& p, Point center, int radius, Vec3 lightDir, double rotation = 0.0);
void drawRings(Canvas& c, Point center, int planetRadius, double tilt, Half half);

void drawOrbit(Canvas& c, const OrbitView& v, double rx, Color col, double alpha, Half half);
// A fading arc behind the planet. `arc` is its angular length (radians).
void drawTrail(Canvas& c, const OrbitView& v, double rx, double angle, double arc, Color col, double maxAlpha,
               Half half);

// Soft atmospheric glow outside the limb (Venus, Earth, Mars, ice giants).
void drawAtmosphereHalo(Canvas& c, const PlanetInfo& p, Point center, int radius, Vec3 lightDir);

// Panels
void drawPanel(Canvas& c, int x, int y, int w, int h, int chamfer, double fillAlpha);
void drawCloseButton(Canvas& c, int cx, int cy, int r, double alpha = 1.0);
// The click-to-learn panel: type, name, five stats and one fact. `alpha`
// fades the whole panel in during the zoom transition.
void drawFactPanel(Canvas& c, int x, int y, int w, int h, const FactSheet& f, double alpha = 1.0);

// Orbital angle of a planet after `day` days. Planets orbit prograde
// (anticlockwise seen from above the north pole), which in this screen
// convention means the angle decreases with time.
double planetAngle(const PlanetInfo& p, double day);


}  // namespace galaxis
