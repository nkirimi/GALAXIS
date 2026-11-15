// Renderer.cpp - CP3 OpenGL renderer.
#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "Shaders.h"
#include "galaxis/Components.h"
#include "galaxis/Facts.h"
#include "galaxis/Font.h"
#include "galaxis/Texture.h"

namespace galaxis {

namespace {

constexpr int kOrbitSegments = 256;
constexpr int kTrailSegments = 48;
constexpr double kTrailArc = 0.6;

// UV sphere matching the planet texture mapping used by surfaceColor():
// u = (lon + pi) / 2pi, v = (lat + pi/2) / pi, position = (cos lat sin lon, sin lat, cos lat cos lon).
void buildSphere(gl::Mesh& m, int stacks, int slices) {
    std::vector<float> v;
    std::vector<unsigned> idx;
    for (int i = 0; i <= stacks; ++i) {
        const double lat = -kPi / 2 + kPi * i / stacks;
        for (int j = 0; j <= slices; ++j) {
            const double lon = -kPi + 2 * kPi * j / slices;
            const float x = static_cast<float>(std::cos(lat) * std::sin(lon));
            const float y = static_cast<float>(std::sin(lat));
            const float z = static_cast<float>(std::cos(lat) * std::cos(lon));
            v.insert(v.end(), {x, y, z, x, y, z, static_cast<float>(j) / slices, static_cast<float>(i) / stacks});
        }
    }
    for (int i = 0; i < stacks; ++i)
        for (int j = 0; j < slices; ++j) {
            const unsigned a = static_cast<unsigned>(i * (slices + 1) + j), b = a + 1;
            const unsigned c = a + static_cast<unsigned>(slices + 1), d = c + 1;
            idx.insert(idx.end(), {a, b, d, a, d, c});  // counter-clockwise from outside
        }
    m.upload(v, {3, 3, 2}, idx, GL_TRIANGLES);
}

void buildRing(gl::Mesh& m, int segments) {
    std::vector<float> v;
    for (int k = 0; k <= segments; ++k) {
        const double a = 2 * kPi * k / segments;
        for (double rho : {kRingInner, kRingOuter})
            v.insert(v.end(), {static_cast<float>(std::cos(a) * rho), 0.f, static_cast<float>(std::sin(a) * rho),
                               static_cast<float>(rho)});
    }
    m.upload(v, {3, 1}, {}, GL_TRIANGLE_STRIP);
}

// Equirectangular image of a colour function f(lon, lat).
template <typename F>
std::vector<unsigned char> bake(int w, int h, F f) {
    std::vector<unsigned char> img(static_cast<std::size_t>(w) * h * 4);
    for (int j = 0; j < h; ++j) {
        const double lat = ((j + 0.5) / h - 0.5) * kPi;
        for (int i = 0; i < w; ++i) {
            const double lon = ((i + 0.5) / w) * 2 * kPi - kPi;
            const Color c = f(lon, lat);
            const std::size_t k = (static_cast<std::size_t>(j) * w + i) * 4;
            img[k] = c.r;
            img[k + 1] = c.g;
            img[k + 2] = c.b;
            img[k + 3] = 255;
        }
    }
    return img;
}

// Draws text art with CP1 code on black and white canvases, then recovers alpha.
template <typename F>
std::vector<unsigned char> paint(int w, int h, F f) {
    Canvas black(w, h, {0, 0, 0}), white(w, h, {255, 255, 255});
    f(black);
    f(white);
    return gl::matteToRGBA(black, white);
}

}  // namespace

// ================================================================ setup
bool GLRenderer::init(std::string& error) {
    using namespace shaders;
    bool ok = sky_.build(kSkyVS, kSkyFS, error) && stars_.build(kStarVS, kStarFS, error) &&
              planet_.build(kBodyVS, kPlanetFS, error) && sun_.build(kBodyVS, kSunFS, error) &&
              glow_.build(kGlowVS, kGlowFS, error) && line_.build(kLineVS, kLineFS, error) &&
              ring_.build(kRingVS, kRingFS, error) && quad_.build(kQuadVS, kQuadFS, error);
    if (!ok) return false;

    glGenVertexArrays(1, &emptyVao_);
    buildSphere(sphere_, 64, 128);
    buildRing(ringMesh_, 192);
    billboard_.upload({-1, -1, 1, -1, -1, 1, 1, 1}, {2}, {}, GL_TRIANGLE_STRIP);
    quadMesh_.upload({0, 0, 1, 0, 0, 1, 1, 1}, {2}, {}, GL_TRIANGLE_STRIP);

    // Orbits: one circle per planet in a single buffer.
    std::vector<float> orbit;
    const auto& sys = solarSystem();
    for (int i = 0; i < 8; ++i) {
        orbitFirst_[static_cast<std::size_t>(i)] = static_cast<int>(orbit.size() / 4);
        const double r = orbitRadiusWorld(sys[static_cast<std::size_t>(i)].orbitAU);
        for (int k = 0; k <= kOrbitSegments; ++k) {
            const double a = 2 * kPi * k / kOrbitSegments;
            orbit.insert(orbit.end(), {static_cast<float>(r * std::cos(a)), 0.f, static_cast<float>(r * std::sin(a)), 1.f});
        }
    }
    orbitCount_ = kOrbitSegments + 1;
    orbitMesh_.upload(orbit, {3, 1}, {}, GL_LINE_STRIP);
    trailMesh_.upload(std::vector<float>(static_cast<std::size_t>(8 * (kTrailSegments + 1) * 4), 0.f), {3, 1}, {},
                      GL_LINE_STRIP, GL_STREAM_DRAW);

    std::vector<float> stars;
    for (const Star& s : starCatalog())
        stars.insert(stars.end(), {static_cast<float>(s.dir.x), static_cast<float>(s.dir.y), static_cast<float>(s.dir.z),
                                   static_cast<float>(s.brightness), s.tint.r / 255.f, s.tint.g / 255.f, s.tint.b / 255.f});
    starMesh_.upload(stars, {3, 1, 3}, {}, GL_POINTS);
    return true;
}

void GLRenderer::bakeTextures() {
    const auto& sys = solarSystem();
    for (int i = 0; i < 8; ++i) {
        const PlanetInfo& p = sys[static_cast<std::size_t>(i)];
        const int w = (i == 2 || i == 4) ? 2048 : 1024;  // Earth and Jupiter get extra detail
        bodyTex_[static_cast<std::size_t>(i)] =
            gl::makeTexture(w, w / 2, bake(w, w / 2, [&](double lon, double lat) { return surfaceColor(p, lon, lat); }), true, true);
    }
    bodyTex_[kSunIndex] = gl::makeTexture(1024, 512, bake(1024, 512, sunSurfaceColor), true, true);
    skyTex_ = gl::makeTexture(1024, 512, bake(1024, 512, [](double lon, double lat) {
                                  return skyColor({std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon)});
                              }),
                              false, true);
    std::vector<unsigned char> ring(256 * 4);
    for (int i = 0; i < 256; ++i) {
        const double rho = kRingInner + (kRingOuter - kRingInner) * (i + 0.5) / 256.0;
        const Color c = ringColor(rho);
        ring[static_cast<std::size_t>(i) * 4 + 0] = c.r;
        ring[static_cast<std::size_t>(i) * 4 + 1] = c.g;
        ring[static_cast<std::size_t>(i) * 4 + 2] = c.b;
        ring[static_cast<std::size_t>(i) * 4 + 3] = static_cast<unsigned char>(ringAlpha(rho) * 255);
    }
    ringTex_ = gl::makeTexture(256, 1, ring, false, false);
    baked_ = true;
}

// ================================================================ UI images
GLuint GLRenderer::panelTexture(int body) {
    GLuint& t = panelTex_[static_cast<std::size_t>(body)];
    if (!t) {
        const auto rgba = paint(480, 560, [&](Canvas& c) { drawFactPanel(c, 0, 0, 480, 560, factSheet(body), 1.0); });
        t = gl::makeTexture(480, 560, rgba, false, false);
    }
    return t;
}

GLuint GLRenderer::textTexture(const std::string& key, int& w, int& h) {
    auto it = text_.find(key);
    if (it == text_.end()) {
        std::vector<unsigned char> rgba;
        if (key == "splash") {
            w = 760;
            h = 220;
            rgba = paint(w, h, [&](Canvas& c) {
                const std::string title = "GALAXIS";
                drawText(c, (w - textWidth(title, 12)) / 2, 20, title, colors::White, 12);
                const std::string sub = "A Procedural Solar System Simulator";
                drawText(c, (w - textWidth(sub, 3)) / 2, 128, sub, {150, 190, 255}, 3);
                const std::string hint = "Press H for controls";
                drawText(c, (w - textWidth(hint, 2)) / 2, 182, hint, {120, 135, 170}, 2);
            });
        } else if (key == "help") {
            const char* rows[][2] = {{"Drag", "Orbit the camera"},
                                     {"Right-drag", "Pan"},
                                     {"Scroll", "Zoom"},
                                     {"Click a body", "Fly in and show its facts"},
                                     {"Esc / click X", "Back to the overview"},
                                     {"1-8, 0", "Jump to a planet / the Sun"},
                                     {"Space", "Pause / play"},
                                     {"+ / -", "Faster / slower"},
                                     {"O", "Orbit lines on / off"},
                                     {"R", "Reset the camera"},
                                     {"F", "Fullscreen"},
                                     {"H", "Hide this help"}};
            w = 580;
            h = 60 + 12 * 22 + 14;
            rgba = paint(w, h, [&](Canvas& c) {
                drawPanel(c, 0, 0, w, h, 14, 0.85);
                drawText(c, 24, 20, "CONTROLS", colors::Accent, 2);
                int y = 56;
                for (const auto& r : rows) {
                    drawText(c, 24, y, r[0], {235, 240, 255}, 2);
                    drawText(c, 214, y, r[1], {150, 165, 200}, 2);
                    y += 22;
                }
            });
        } else {  // toast
            const std::string msg = key.substr(6);
            w = textWidth(msg, 2) + 40;
            h = 38;
            rgba = paint(w, h, [&](Canvas& c) {
                drawPanel(c, 0, 0, w, h, 8, 0.85);
                drawText(c, 20, 12, msg, {235, 240, 255}, 2);
            });
        }
        it = text_.emplace(key, TextImage{gl::makeTexture(w, h, rgba, false, false), w, h}).first;
    }
    w = it->second.w;
    h = it->second.h;
    return it->second.tex;
}

PixelRect GLRenderer::panelRect(const Galaxis& g, int fbW, int fbH) const {
    const double s = fbH / 720.0;
    const double slide = (1.0 - g.panelAlpha()) * 48.0;
    return {fbW - (480.0 + 60.0 - slide) * s, (fbH - 560.0 * s) / 2, 480.0 * s, 560.0 * s};
}

bool GLRenderer::hitsCloseButton(const Galaxis& g, double px, double py, int fbW, int fbH) const {
    if (g.panelAlpha() < 0.5) return false;
    const PixelRect r = panelRect(g, fbW, fbH);
    const double s = fbH / 720.0;
    const double cx = r.x + (480 - 38) * s, cy = r.y + 38 * s;
    return std::hypot(px - cx, py - cy) <= 22 * s;
}

// ================================================================ drawing
void GLRenderer::drawQuad(const PixelRect& r, int fbW, int fbH, GLuint tex, Color color, double alpha) {
    quad_.use();
    const float x = static_cast<float>(r.x / fbW * 2 - 1);
    const float y = static_cast<float>(1 - (r.y + r.h) / fbH * 2);
    quad_.set4("uRect", x, y, static_cast<float>(r.w / fbW * 2), static_cast<float>(r.h / fbH * 2));
    quad_.set("uUseTex", tex ? 1 : 0);
    quad_.set("uColor", color, static_cast<float>(alpha));
    if (tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        quad_.set("uTex", 0);
    }
    quadMesh_.draw();
}

void GLRenderer::drawSky(const CameraPose& pose, double aspect) {
    sky_.use();
    sky_.set("uRight", pose.right());
    sky_.set("uUp", pose.up());
    sky_.set("uFwd", pose.forward());
    sky_.set("uTan", static_cast<float>(std::tan(kFovY / 2)));
    sky_.set("uAspect", static_cast<float>(aspect));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyTex_);
    sky_.set("uSky", 0);
    glBindVertexArray(emptyVao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GLRenderer::drawStars(const Mat4& view, const Mat4& proj, int fbHeight) {
    stars_.use();
    stars_.set("uView", view);
    stars_.set("uProj", proj);
    stars_.set("uScale", static_cast<float>(std::max(1.0, fbHeight / 720.0)));
    glBlendFunc(GL_ONE, GL_ONE);  // additive
    starMesh_.draw();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GLRenderer::drawBody(const Galaxis& g, int b, const Mat4& viewProj, const CameraPose& pose) {
    const BodyState s = g.body(b);
    // Model matrix: translate * (tilt * spin) * scale.
    const Mat4 model = translate3D(s.position) * s.orientation * scale3D({s.radius, s.radius, s.radius});
    const gl::Shader& sh = (b == kSunIndex) ? sun_ : planet_;
    sh.use();
    sh.set("uModel", model);
    sh.set("uViewProj", viewProj);
    sh.set("uEye", pose.eye);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bodyTex_[static_cast<std::size_t>(b)]);
    sh.set("uTex", 0);
    if (b != kSunIndex) {
        const PlanetInfo& p = solarSystem()[static_cast<std::size_t>(b)];
        sh.set("uAtmosphere", p.atmosphere, static_cast<float>(p.atmosphereStrength));
        sh.set("uSpecular", b == 2 ? 0.55f : 0.06f);
        sh.set("uHasRings", p.rings ? 1 : 0);
        sh.set("uCenter", s.position);
        sh.set("uRingNormal", s.axis);
        sh.set("uRadius", static_cast<float>(s.radius));
        sh.set2("uRingRange", static_cast<float>(kRingInner), static_cast<float>(kRingOuter));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ringTex_);
        sh.set("uRingTex", 1);
    }
    sphere_.draw();
}

void GLRenderer::drawBodyExtras(const Galaxis& g, int b, const Mat4& viewProj, const CameraPose& pose) {
    const BodyState s = g.body(b);
    glDepthMask(GL_FALSE);
    if (b == kSunIndex) {
        glBlendFunc(GL_ONE, GL_ONE);
        glow_.use();
        glow_.set("uViewProj", viewProj);
        glow_.set("uCenter", s.position);
        glow_.set("uRight", pose.right());
        glow_.set("uUp", pose.up());
        glow_.set("uSize", static_cast<float>(s.radius * 3.4));
        glow_.set("uMode", 0);
        glow_.set("uRadius", static_cast<float>(s.radius));
        glow_.set("uColor", Color{255, 150, 60}, 1.0f);
        billboard_.draw();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        const PlanetInfo& p = solarSystem()[static_cast<std::size_t>(b)];
        if (p.rings) {
            glDisable(GL_CULL_FACE);
            ring_.use();
            ring_.set("uModel", translate3D(s.position) * rotateZ(-axialTiltRadians(b)) * scale3D({s.radius, s.radius, s.radius}));
            ring_.set("uViewProj", viewProj);
            ring_.set("uCenter", s.position);
            ring_.set("uNormal", s.axis);
            ring_.set("uRadius", static_cast<float>(s.radius));
            ring_.set2("uRingRange", static_cast<float>(kRingInner), static_cast<float>(kRingOuter));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ringTex_);
            ring_.set("uRingTex", 0);
            ringMesh_.draw();
            glEnable(GL_CULL_FACE);
        }
        if (p.atmosphereStrength > 0) {
            // Direction of the Sun as seen on screen around this planet.
            const Vec3 L = normalize(-s.position);
            const double lx = dot(L, pose.right()), ly = dot(L, pose.up());
            const double ll = std::max(1e-6, std::hypot(lx, ly));
            glBlendFunc(GL_ONE, GL_ONE);
            glow_.use();
            glow_.set("uViewProj", viewProj);
            glow_.set("uCenter", s.position);
            glow_.set("uRight", pose.right());
            glow_.set("uUp", pose.up());
            glow_.set("uSize", static_cast<float>(s.radius * 1.25));
            glow_.set("uMode", 1);
            glow_.set("uRadius", static_cast<float>(s.radius));
            glow_.set("uColor", p.atmosphere, static_cast<float>(p.atmosphereStrength));
            glow_.set2("uLight2D", static_cast<float>(lx / ll), static_cast<float>(ly / ll));
            billboard_.draw();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }
    glDepthMask(GL_TRUE);
}

void GLRenderer::drawOrbits(const Galaxis& g, const Mat4& viewProj, const CameraPose& pose) {
    if (!g.showOrbits) return;
    line_.use();
    line_.set("uViewProj", viewProj);
    line_.set("uEye", pose.eye);
    line_.set("uSunDist", static_cast<float>(std::max(1.0, length(pose.eye))));
    glDepthMask(GL_FALSE);
    line_.set("uColor", colors::Orbit, 0.42f);
    for (int i = 0; i < 8; ++i) orbitMesh_.drawRange(orbitFirst_[static_cast<std::size_t>(i)], orbitCount_);

    // Trails: an arc behind each planet whose alpha fades to zero.
    std::vector<float> v;
    const auto& sys = solarSystem();
    for (int i = 0; i < 8; ++i) {
        const double r = orbitRadiusWorld(sys[static_cast<std::size_t>(i)].orbitAU);
        const double a = orbitAngle(i, g.day);
        for (int k = 0; k <= kTrailSegments; ++k) {
            const double t = static_cast<double>(k) / kTrailSegments;
            const double ang = a + kTrailArc * t;
            v.insert(v.end(), {static_cast<float>(r * std::cos(ang)), 0.f, static_cast<float>(r * std::sin(ang)),
                               static_cast<float>(std::pow(1.0 - t, 1.4))});
        }
    }
    trailMesh_.update(v);
    for (int i = 0; i < 8; ++i) {
        line_.set("uColor", mix(sys[static_cast<std::size_t>(i)].base, colors::White, 0.35), 0.75f);
        trailMesh_.drawRange(i * (kTrailSegments + 1), kTrailSegments + 1);
    }
    glDepthMask(GL_TRUE);
}

void GLRenderer::render(const Galaxis& g, const UiState& ui, int fbW, int fbH) {
    glViewport(0, 0, fbW, fbH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    if (baked_) {
        const CameraPose pose = g.pose();
        const Mat4 view = pose.view(), proj = g.projection(), viewProj = proj * view;
        const double focus = g.focusAmount();
        const int f = g.focus();
        auto skip = [&](int b) { return b == f && focus > 0; };

        glDisable(GL_DEPTH_TEST);
        drawSky(pose, static_cast<double>(fbW) / fbH);
        drawStars(view, proj, fbH);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        for (int b = 0; b < kBodyCount; ++b)
            if (!skip(b)) drawBody(g, b, viewProj, pose);
        drawOrbits(g, viewProj, pose);
        for (int b = 0; b < kBodyCount; ++b)
            if (!skip(b)) drawBodyExtras(g, b, viewProj, pose);

        if (f >= 0 && focus > 0) {
            // Dim everything else, then draw the chosen body on top.
            glDisable(GL_DEPTH_TEST);
            drawQuad({0, 0, double(fbW), double(fbH)}, fbW, fbH, 0, {2, 3, 9}, 0.90 * focus);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_DEPTH_BUFFER_BIT);
            drawBody(g, f, viewProj, pose);
            drawBodyExtras(g, f, viewProj, pose);
        }
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        if (f >= 0 && g.panelAlpha() > 0)
            drawQuad(panelRect(g, fbW, fbH), fbW, fbH, panelTexture(f), colors::White, g.panelAlpha());
    }
    glDisable(GL_DEPTH_TEST);

    const double s = fbH / 720.0;
    int w = 0, h = 0;
    if (ui.blackout > 0) drawQuad({0, 0, double(fbW), double(fbH)}, fbW, fbH, 0, {0, 0, 0}, ui.blackout);
    if (ui.splash > 0) {
        const GLuint t = textTexture("splash", w, h);
        drawQuad({(fbW - w * s) / 2, fbH * 0.40 - h * s / 2, w * s, h * s}, fbW, fbH, t, colors::White, ui.splash);
    }
    if (ui.help) {
        const GLuint t = textTexture("help", w, h);
        drawQuad({24 * s, 24 * s, w * s, h * s}, fbW, fbH, t, colors::White, 1.0);
    }
    if (ui.toastAlpha > 0 && !ui.toast.empty()) {
        const GLuint t = textTexture("toast:" + ui.toast, w, h);
        drawQuad({(fbW - w * s) / 2, fbH - (h + 36) * s, w * s, h * s}, fbW, fbH, t, colors::White, ui.toastAlpha);
    }
}

}  // namespace galaxis
