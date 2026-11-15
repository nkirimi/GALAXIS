// Planet.h - data for the eight planets: real physical values (for the
// click-to-learn info card) plus the look used to render each one.
#pragma once

#include <array>
#include <string>

#include "galaxis/Color.h"

namespace galaxis {

enum class Surface { Rocky, Banded, Ocean, Ice };

struct PlanetInfo {
    // --- facts shown in the info card
    std::string name;
    std::string type;
    double orbitAU;        // mean distance from the Sun
    double periodDays;     // orbital period (one year on that planet)
    double radiusKm;       // mean radius
    double rotationHours;  // sidereal rotation; negative = retrograde

    // --- rendering
    int displayRadius;     // on-screen radius in the main scene, pixels
    Surface surface;
    Color base;
    Color accent;
    double bandFreq;       // cloud-band frequency for banded / icy planets
    Color atmosphere;
    double atmosphereStrength;
    bool rings;
    bool greatSpot;
    double phase0;         // orbital angle (radians) at day 0 of the simulation
};

// Mercury..Neptune, in order from the Sun.
const std::array<PlanetInfo, 8>& solarSystem();

}  // namespace galaxis
