// Demos.h - one test image per CP1 technique, for the report / marking.
#pragma once

#include "galaxis/Canvas.h"

namespace galaxis {

Canvas demoLines();       // 01: DDA vs Bresenham
Canvas demoCircles();     // 02: midpoint circle, symmetry, spans, shading
Canvas demoEllipses();    // 03: midpoint ellipse, orbit halves, rings
Canvas demoFills();       // 04: scanline polygon, boundary fill, flood fill
Canvas demoClipping();    // 05: Cohen-Sutherland
Canvas demoComponents();  // 06: every Galaxis component on one sheet

}  // namespace galaxis
