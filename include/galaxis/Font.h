// Font.h - a hand-made 5x7 bitmap font (upper and lower case, digits,
// punctuation) so the info panel can show text without any external library.
#pragma once

#include <string>
#include <vector>

#include "galaxis/Canvas.h"

namespace galaxis {

// Draws text with its top-left corner at (x, y). Each font pixel becomes a
// scale x scale block. '\n' starts a new line.
void drawText(Canvas& c, int x, int y, const std::string& text, Color col, int scale = 1, double alpha = 1.0);

constexpr int kLineHeight = 11;  // rows per text line, including descender and gap

int textWidth(const std::string& text, int scale = 1);
inline int textHeight(int scale = 1) { return 7 * scale; }

// Splits text into lines no wider than maxWidth pixels, breaking at spaces.
std::vector<std::string> wrapText(const std::string& text, int maxWidth, int scale = 1);

}  // namespace galaxis
