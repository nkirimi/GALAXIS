// Scene.h - the two Galaxis views.
//
//  * Overview: only the Sun, the planets, their orbits and the starfield.
//    No text or panels.
//  * Focus: after a body is clicked it glides (Bezier path) to the left side
//    of the screen and grows to fill it, the rest of the system dims, and
//    the facts panel fades in on the right.
//
// SceneState is everything that changes over time. CP1 renders single
// states; CP2 animates `day` (orbits) and `zoom` (the click transition).
#pragma once

#include "galaxis/Canvas.h"

namespace galaxis {

struct SceneState {
    double day = 0.0;         // simulation time in Earth days
    bool showOrbits = true;   // orbit lines + trails (toggleable, as in the proposal)
    int focus = -1;           // body being viewed: -1 none, 0..7 Mercury..Neptune, 8 the Sun
    double zoom = 1.0;        // focus transition: 0 = still in orbit, 1 = fully zoomed in
    double spin = 0.0;        // extra surface rotation (radians) for the focused planet
};

constexpr int kSceneWidth = 1280;
constexpr int kSceneHeight = 720;

void renderScene(Canvas& c, const SceneState& s);

// Click hit-test for the overview: the body under screen point (x, y),
// taking draw order into account (front-most wins), or -1 for empty space.
int pickBody(const SceneState& s, int x, int y);

}  // namespace galaxis
