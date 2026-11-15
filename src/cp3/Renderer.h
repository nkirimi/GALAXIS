// Renderer.h - CP3 OpenGL renderer for a Galaxis world.
//
// Independent of the windowing library: main.cpp drives it from GLFW, and it
// can equally be driven by a headless test harness.
#pragma once

#include <array>
#include <map>
#include <string>

#include "GLUtil.h"
#include "galaxis/World.h"

namespace galaxis {

// Overlays that are not part of the 3-D world.
struct UiState {
    double splash = 0.0;       // title card opacity (0..1)
    double blackout = 0.0;     // black layer over the scene during the intro
    std::string toast;         // short status message ("Paused", "20 days / second")
    double toastAlpha = 0.0;
    bool help = false;         // controls overlay
};

struct PixelRect {
    double x, y, w, h;  // framebuffer pixels, origin top-left
    bool contains(double px, double py) const { return px >= x && py >= y && px < x + w && py < y + h; }
};

class GLRenderer {
public:
    // Stage 1: shaders, meshes and the title card (fast).
    bool init(std::string& error);
    // Stage 2: bake the planet, Sun, ring and sky textures with the CP1/CP2
    // procedural colour functions (takes a second or two).
    void bakeTextures();
    bool texturesReady() const { return baked_; }

    void render(const Galaxis& g, const UiState& ui, int fbWidth, int fbHeight);

    // Layout of the facts panel and its close button, for hit-testing clicks.
    PixelRect panelRect(const Galaxis& g, int fbWidth, int fbHeight) const;
    bool hitsCloseButton(const Galaxis& g, double px, double py, int fbWidth, int fbHeight) const;

private:
    void drawSky(const CameraPose& pose, double aspect);
    void drawStars(const Mat4& view, const Mat4& proj, int fbHeight);
    void drawBody(const Galaxis& g, int body, const Mat4& viewProj, const CameraPose& pose);
    void drawBodyExtras(const Galaxis& g, int body, const Mat4& viewProj, const CameraPose& pose);  // rings, halo, corona
    void drawOrbits(const Galaxis& g, const Mat4& viewProj, const CameraPose& pose);
    void drawQuad(const PixelRect& r, int fbW, int fbH, GLuint tex, Color color, double alpha);
    GLuint textTexture(const std::string& key, int& w, int& h);
    GLuint panelTexture(int body);

    gl::Shader sky_, stars_, planet_, sun_, glow_, line_, ring_, quad_;
    gl::Mesh sphere_, ringMesh_, orbitMesh_, trailMesh_, starMesh_, billboard_, quadMesh_;
    GLuint emptyVao_ = 0;
    std::array<GLuint, kBodyCount> bodyTex_{};
    GLuint ringTex_ = 0, skyTex_ = 0;
    std::array<GLuint, kBodyCount> panelTex_{};
    struct TextImage {
        GLuint tex;
        int w, h;
    };
    std::map<std::string, TextImage> text_;
    std::array<int, 8> orbitFirst_{};
    int orbitCount_ = 0;
    bool baked_ = false;
};

}  // namespace galaxis
