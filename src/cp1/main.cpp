// Galaxis - CP1 Technical Foundation
// Pure C++17 (standard library only). Renders the technique test sheets,
// the clean overview, the focus view of every body, and a zoom sequence that
// shows what happens when a planet is clicked.
//
// Usage:  galaxis_cp1 [--ppm] [--out DIR]
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>

#include "galaxis/Canvas.h"
#include "galaxis/Demos.h"
#include "galaxis/Facts.h"
#include "galaxis/Scene.h"

using namespace galaxis;

namespace {

std::string slug(const std::string& name) {
    std::string s;
    for (char ch : name) {
        if (ch == ' ') continue;
        s += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s == "thesun" ? "sun" : s;
}

}  // namespace

int main(int argc, char** argv) {
    bool alsoPPM = false;
    std::string outDir = "output/cp1";
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--ppm") {
            alsoPPM = true;
        } else if (arg == "--out" && i + 1 < argc) {
            outDir = argv[++i];
        } else {
            std::cerr << "usage: " << argv[0] << " [--ppm] [--out DIR]\n";
            return 1;
        }
    }

    std::error_code ec;
    std::filesystem::create_directories(outDir + "/focus", ec);
    std::filesystem::create_directories(outDir + "/zoom", ec);

    int failures = 0;
    auto save = [&](const std::string& name, const Canvas& c) {
        const std::string base = outDir + "/" + name;
        bool ok = c.saveBMP(base + ".bmp");
        if (alsoPPM) ok = c.savePPM(base + ".ppm") && ok;
        std::cout << (ok ? "  wrote " : "  FAILED ") << base << ".bmp\n";
        if (!ok) ++failures;
    };
    auto render = [](const SceneState& s) {
        Canvas c(kSceneWidth, kSceneHeight);
        renderScene(c, s);
        return c;
    };

    std::cout << "Galaxis CP1 - rendering...\n";

    // Technique test sheets
    save("01_lines", demoLines());
    save("02_circles", demoCircles());
    save("03_ellipses", demoEllipses());
    save("04_fills", demoFills());
    save("05_clipping", demoClipping());
    save("06_components", demoComponents());

    // Main view: only the Sun, planets, orbits and stars.
    save("galaxis_scene", render(SceneState{}));

    // Focus view for every body (what the screen shows after a click).
    for (int body = 0; body < kBodyCount; ++body) {
        SceneState s;
        s.focus = body;
        s.zoom = 1.0;
        save("focus/" + slug(factSheet(body).name), render(s));
    }

    // The click transition for Earth, at five points in time, plus a
    // contact sheet of all five frames. Also checks the click hit-test.
    SceneState probe;
    std::cout << "  pickBody at Earth's position -> " << pickBody(probe, 771, 373) << " (expect 2)\n";

    const int frames = 5;
    const int thumb = 4;  // contact sheet thumbnails are 1/4 size
    Canvas sheet(frames * (kSceneWidth / thumb) + (frames + 1) * 8, kSceneHeight / thumb + 16, {12, 16, 30});
    for (int i = 0; i < frames; ++i) {
        SceneState s;
        s.focus = 2;
        s.zoom = static_cast<double>(i) / (frames - 1);
        const Canvas frame = render(s);
        save("zoom/earth_" + std::to_string(i), frame);
        sheet.blit(frame.downscaled(thumb), 8 + i * (kSceneWidth / thumb + 8), 8);
    }
    save("zoom_sequence", sheet);

    std::cout << (failures ? "Done with errors.\n" : "Done.\n");
    return failures ? 1 : 0;
}
