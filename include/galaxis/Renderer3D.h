// Renderer3D.h - CP2 software 3-D renderer.
//
// Draws a Galaxis world into a Canvas using only the CP1 rasterisers
// (Bresenham, circle spans, scanline fill, Cohen-Sutherland, bitmap font) and
// the CP2 matrices: every point goes world -> view -> clip -> NDC -> screen.
// Visibility uses the painter's algorithm: orbit segments, trails and bodies
// are sorted by depth and drawn back to front.
#pragma once

#include "galaxis/Canvas.h"
#include "galaxis/Transform.h"
#include "galaxis/World.h"

namespace galaxis {

// The full viewing pipeline for one frame.
struct Projector {
    Mat4 view, proj, viewProj, screen;  // screen = viewport * proj * view
    int width = 0, height = 0;
    double focalPx = 0;  // pixels per unit at distance 1 (vertical)
    CameraPose pose;

    Projector(const CameraPose& pose, double fovY, int width, int height);

    Vec3 toView(Vec3 world) const { return transformPoint(view, world); }
    // Projects a view-space point to pixels. Returns false if it is behind
    // the near plane.
    bool toScreen(Vec3 viewPoint, Point& out) const;
    Vec3 rayDir(double px, double py) const;  // world-space direction through a pixel
};

struct RenderOptions {
    bool drawPanel = true;  // facts panel in focus mode
};

void renderWorld(Canvas& c, const Galaxis& g, const RenderOptions& opt = {});

// Pieces reused by the CP2 demo sheets.
void drawSky(Canvas& c, const Projector& pr);
void drawBody(Canvas& c, const Projector& pr, const Galaxis& g, int body);

}  // namespace galaxis
