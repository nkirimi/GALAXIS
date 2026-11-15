// Planet.cpp - planet table. Physical values are standard mean values
// (NASA planetary fact sheet); colours are artistic.
#include "galaxis/Planet.h"

namespace galaxis {

const std::array<PlanetInfo, 8>& solarSystem() {
    static const std::array<PlanetInfo, 8> kPlanets = {{
        //  name       type           AU      period     radius    rotation
        {"MERCURY", "TERRESTRIAL", 0.387, 87.97, 2439.7, 1407.6,
         4, Surface::Rocky, {160, 152, 145}, {95, 90, 88}, 0, {0, 0, 0}, 0.0, false, false, 5.00},
        {"VENUS", "TERRESTRIAL", 0.723, 224.70, 6051.8, -5832.5,
         7, Surface::Banded, {232, 200, 140}, {205, 165, 105}, 3, {255, 225, 170}, 0.45, false, false, 3.40},
        {"EARTH", "TERRESTRIAL", 1.000, 365.26, 6371.0, 23.93,
         7, Surface::Ocean, {35, 85, 185}, {70, 135, 65}, 0, {120, 175, 255}, 0.85, false, false, 0.25},
        {"MARS", "TERRESTRIAL", 1.524, 686.98, 3389.5, 24.62,
         5, Surface::Rocky, {200, 95, 55}, {125, 55, 35}, 0, {235, 155, 115}, 0.25, false, false, 4.00},
        {"JUPITER", "GAS GIANT", 5.203, 4332.59, 69911.0, 9.93,
         17, Surface::Banded, {222, 190, 150}, {150, 105, 72}, 14, {0, 0, 0}, 0.0, false, true, 5.60},
        {"SATURN", "GAS GIANT", 9.537, 10759.22, 58232.0, 10.66,
         14, Surface::Banded, {228, 205, 155}, {185, 150, 100}, 10, {0, 0, 0}, 0.0, true, false, 2.70},
        {"URANUS", "ICE GIANT", 19.191, 30688.5, 25362.0, -17.24,
         10, Surface::Ice, {155, 220, 230}, {115, 190, 205}, 6, {175, 235, 245}, 0.5, false, false, 3.60},
        {"NEPTUNE", "ICE GIANT", 30.069, 60182.0, 24622.0, 16.11,
         10, Surface::Ice, {60, 105, 225}, {35, 65, 175}, 8, {110, 155, 255}, 0.5, false, false, 0.35},
    }};
    return kPlanets;
}

}  // namespace galaxis
