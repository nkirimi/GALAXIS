// Clip.h - Cohen-Sutherland line clipping against an axis-aligned window.
#pragma once

#include "galaxis/Canvas.h"

namespace galaxis {

struct ClipRect {
    double xmin, ymin, xmax, ymax;
};

// 4-bit region codes, TBRL order. Screen y grows downwards, so "TOP" means
// y < ymin.
enum OutCode : int { kInside = 0, kLeft = 1, kRight = 2, kBottom = 4, kTop = 8 };

int outCode(double x, double y, const ClipRect& r);

// Clips the segment in place. Returns false if it lies completely outside.
bool cohenSutherlandClip(double& x0, double& y0, double& x1, double& y1, const ClipRect& r);

// Clip, then rasterise the visible part with Bresenham.
void drawLineClipped(Canvas& c, double x0, double y0, double x1, double y1, const ClipRect& r, Color col,
                     double alpha = 1.0);

}  // namespace galaxis
