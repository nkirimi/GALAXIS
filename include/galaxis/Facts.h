// Facts.h - the click-to-learn content for the 9 bodies (Sun + 8 planets),
// in the format from the proposal (slide 5): a type label, five quick stats
// and one memorable fact.
#pragma once

#include <array>
#include <string>
#include <utility>

#include "galaxis/Color.h"

namespace galaxis {

constexpr int kBodyCount = 9;
constexpr int kSunIndex = 8;  // bodies 0..7 are the planets, Mercury..Neptune

struct FactSheet {
    std::string name;
    std::string kind;                                        // e.g. "Terrestrial planet"
    std::array<std::pair<std::string, std::string>, 5> stats;  // label, value
    std::string fact;                                        // one memorable fact
    Color accent;                                            // tint for the panel
};

const FactSheet& factSheet(int body);

}  // namespace galaxis
