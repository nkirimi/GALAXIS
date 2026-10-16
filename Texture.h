// Texture.h - procedural surface colours shared by every checkpoint.
//
// CP1/CP2 call these per pixel while scan-converting a sphere; CP3 calls the
// same functions once at start-up to bake OpenGL textures, so the planets
// look the same in all three versions.
#pragma once

#include <vector>

#include "galaxis/Color.h"
#include "galaxis/Math.h"
#include "galaxis/Planet.h"

namespace galaxis {

// Value noise on an integer lattice, summed over octaves (fractal Brownian motion).
double valueNoise3(double x, double y, double z, unsigned seed);
double fbm3(double x, double y, double z, unsigned seed, int octaves);
double fbm2(double x, double y, unsigned seed, int octaves);

unsigned planetSeed(const PlanetInfo& p);

// Surface colour at a longitude/latitude (radians) in the planet's own frame
// (y = spin axis). Sampling noise at the 3-D point avoids a seam.
Color surfaceColor(const PlanetInfo& p, double lon, double lat);

// Sun photosphere colour (granulation only; limb darkening is applied by the renderer).
Color sunSurfaceColor(double lon, double lat);

// Saturn's rings: colour and opacity at a radius given in planet radii.
constexpr double kRingInner = 1.32;
constexpr double kRingOuter = 2.25;
Color ringColor(double rho);
double ringAlpha(double rho);

// Background sky (faint nebula) in a world-space direction.
Color skyColor(Vec3 dir);

// Fixed star catalogue: unit directions with brightness and tint, identical
// in CP2 (projected points) and CP3 (GL points).
struct Star {
    Vec3 dir;
    double brightness;  // 0..1
    Color tint;
};
const std::vector<Star>& starCatalog();

}  // namespace galaxis
