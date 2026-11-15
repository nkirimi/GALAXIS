// Gif.h - animated GIF writer in pure C++ (no libraries).
//
//  * Palette: median-cut quantisation over sample frames (255 colours; index
//    255 is reserved as "transparent" = unchanged since the last frame).
//  * Ordered (Bayer) dithering, which stays stable from frame to frame.
//  * LZW compression with variable code width (9..12 bits), as the GIF89a
//    specification requires.
#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "galaxis/Canvas.h"

namespace galaxis {

std::vector<Color> medianCutPalette(const std::vector<const Canvas*>& samples, int maxColors);

class GifWriter {
public:
    GifWriter(const std::string& path, int width, int height, std::vector<Color> palette);
    ~GifWriter();
    bool ok() const { return static_cast<bool>(out_); }

    // delay in hundredths of a second
    void addFrame(const Canvas& frame, int delayCs);
    bool close();

private:
    void writeLzw(const std::vector<std::uint8_t>& indices);
    std::uint8_t nearest(Color c) const;

    std::ofstream out_;
    int w_, h_;
    std::vector<Color> palette_;
    std::vector<std::uint8_t> lut_;   // 32x32x32 colour cube -> palette index
    std::vector<std::uint8_t> prev_;  // previous frame's indices
    bool first_ = true;
    bool closed_ = false;
};

}  // namespace galaxis
