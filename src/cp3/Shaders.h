// Shaders.h - GLSL 3.30 core shaders for the CP3 renderer.
#pragma once

namespace galaxis::shaders {

// ------------------------------------------------------------------ sky
// A full-screen triangle; each pixel looks up the nebula texture in the
// direction of its view ray (equirectangular mapping).
inline const char* kSkyVS = R"(#version 330 core
out vec2 vNdc;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2) * 2.0 - 1.0;
    vNdc = p;
    gl_Position = vec4(p, 0.0, 1.0);
}
)";

inline const char* kSkyFS = R"(#version 330 core
in vec2 vNdc;
out vec4 fragColor;
uniform vec3 uRight, uUp, uFwd;
uniform float uTan, uAspect;
uniform sampler2D uSky;
const float PI = 3.14159265;
void main() {
    vec3 d = normalize(uFwd + uRight * vNdc.x * uTan * uAspect + uUp * vNdc.y * uTan);
    vec2 uv = vec2(atan(d.z, d.x) / (2.0 * PI) + 0.5, asin(clamp(d.y, -1.0, 1.0)) / PI + 0.5);
    fragColor = vec4(textureLod(uSky, uv, 0.0).rgb, 1.0);
}
)";

// ---------------------------------------------------------------- stars
inline const char* kStarVS = R"(#version 330 core
layout(location = 0) in vec3 aDir;
layout(location = 1) in float aBright;
layout(location = 2) in vec3 aColor;
uniform mat4 uView, uProj;
uniform float uScale;
out float vBright;
out vec3 vColor;
void main() {
    // Stars are at infinity: only the camera's rotation affects them.
    gl_Position = uProj * vec4(mat3(uView) * aDir * 400.0, 1.0);
    gl_PointSize = uScale * (aBright > 0.72 ? 3.0 + 9.0 * aBright : 1.6 + 1.6 * aBright);
    vBright = aBright;
    vColor = aColor;
}
)";

inline const char* kStarFS = R"(#version 330 core
in float vBright;
in vec3 vColor;
out vec4 fragColor;
void main() {
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    float core = vBright > 0.72 ? exp(-dot(p, p) * 22.0) : smoothstep(1.0, 0.2, length(p));
    float spike = 0.0;
    if (vBright > 0.72)  // four-point sparkle on bright stars
        spike = max(exp(-abs(p.y) * 30.0) * (1.0 - abs(p.x)), exp(-abs(p.x) * 30.0) * (1.0 - abs(p.y))) * 0.6;
    float k = max(core, spike) * vBright;
    fragColor = vec4(vColor * k, 1.0);
}
)";

// --------------------------------------------------------------- bodies
inline const char* kBodyVS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 uModel, uViewProj;
out vec3 vWorld;
out vec3 vNormal;
out vec2 vUV;
void main() {
    vec4 w = uModel * vec4(aPos, 1.0);
    vWorld = w.xyz;
    vNormal = mat3(uModel) * aNormal;  // uniform scale, so no inverse-transpose needed
    vUV = aUV;
    gl_Position = uViewProj * w;
}
)";

// Phong-style lighting from a point light at the Sun (the origin):
// Lambert diffuse with a soft terminator, Blinn-Phong specular (oceans),
// an atmospheric rim, and the shadow of Saturn's rings.
inline const char* kPlanetFS = R"(#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uTex;
uniform vec3 uEye;
uniform vec4 uAtmosphere;   // rgb, strength
uniform float uSpecular;
uniform int uHasRings;
uniform vec3 uCenter;
uniform vec3 uRingNormal;
uniform float uRadius;
uniform sampler2D uRingTex;
uniform vec2 uRingRange;    // inner, outer (planet radii)
void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-vWorld);
    vec3 V = normalize(uEye - vWorld);
    vec3 albedo = texture(uTex, vUV).rgb;

    float lambert = dot(N, L);
    float lit = 0.30 * smoothstep(-0.15, 0.25, lambert) + 0.70 * max(lambert, 0.0);
    vec3 col = albedo * (0.06 + 0.94 * lit);

    float ocean = smoothstep(0.06, 0.20, albedo.b - max(albedo.r, albedo.g));
    vec3 H = normalize(L + V);
    col += vec3(1.0, 0.95, 0.85) * pow(max(dot(N, H), 0.0), 60.0) * uSpecular * (0.15 + 0.85 * ocean) * step(0.0, lambert);

    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.2);
    col += uAtmosphere.rgb * rim * uAtmosphere.a * clamp(lambert + 0.3, 0.0, 1.0);

    if (uHasRings == 1) {
        float dn = dot(L, uRingNormal);
        if (abs(dn) > 1e-4) {
            float t = dot(uCenter - vWorld, uRingNormal) / dn;
            if (t > 0.0) {
                float rho = length(vWorld + L * t - uCenter) / uRadius;
                float u = (rho - uRingRange.x) / (uRingRange.y - uRingRange.x);
                if (u > 0.0 && u < 1.0) col *= 1.0 - 0.6 * texture(uRingTex, vec2(u, 0.5)).a;
            }
        }
    }
    fragColor = vec4(col, 1.0);
}
)";

inline const char* kSunFS = R"(#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uTex;
uniform vec3 uEye;
void main() {
    vec3 N = normalize(vNormal);
    float mu = max(dot(N, normalize(uEye - vWorld)), 0.0);
    float t = sqrt(1.0 - mu * mu);
    vec3 base = texture(uTex, vUV).rgb;
    vec3 hot = mix(vec3(1.0, 0.965, 0.84), base, 0.25 + 0.75 * pow(t, 1.2));
    fragColor = vec4(hot * (0.55 + 0.45 * sqrt(mu)) * 1.05, 1.0);  // limb darkening
}
)";

// ---------------------------------------------------------- billboards
// Camera-facing quads for the Sun's corona and atmospheric halos.
inline const char* kGlowVS = R"(#version 330 core
layout(location = 0) in vec2 aCorner;
uniform vec3 uCenter, uRight, uUp;
uniform float uSize;
uniform mat4 uViewProj;
out vec2 vOff;
void main() {
    vOff = aCorner * uSize;
    gl_Position = uViewProj * vec4(uCenter + (uRight * aCorner.x + uUp * aCorner.y) * uSize, 1.0);
}
)";

inline const char* kGlowFS = R"(#version 330 core
in vec2 vOff;
out vec4 fragColor;
uniform int uMode;          // 0 = Sun corona, 1 = atmosphere halo
uniform float uRadius;
uniform vec4 uColor;        // rgb, strength
uniform vec2 uLight2D;      // direction to the Sun on screen (halo only)
void main() {
    float d = length(vOff) / uRadius;
    float k;
    if (uMode == 0) {
        float t = max(0.0, d - 1.0);
        k = 0.85 * exp(-t / 0.35) + 0.22 * exp(-t / 1.3);
        k *= clamp((3.4 - d) / 0.6, 0.0, 1.0);
    } else {
        if (d <= 1.0) discard;
        float daySide = clamp(dot(normalize(vOff), uLight2D) + 0.35, 0.0, 1.0);
        k = uColor.a * 0.6 * exp(-(d - 1.0) / 0.035) * daySide;
    }
    fragColor = vec4(uColor.rgb * k, 1.0);
}
)";

// ---------------------------------------------------------------- lines
inline const char* kLineVS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aAlpha;
uniform mat4 uViewProj;
uniform vec3 uEye;
out float vAlpha;
out float vDist;
void main() {
    vAlpha = aAlpha;
    vDist = length(aPos - uEye);
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)";

inline const char* kLineFS = R"(#version 330 core
in float vAlpha;
in float vDist;
out vec4 fragColor;
uniform vec4 uColor;
uniform float uSunDist;
void main() {
    float fade = clamp(pow(uSunDist / max(vDist, 0.1), 0.7), 0.55, 1.15);  // nearer = brighter
    fragColor = vec4(uColor.rgb, uColor.a * vAlpha * fade);
}
)";

// ---------------------------------------------------------------- rings
inline const char* kRingVS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aRho;
uniform mat4 uModel, uViewProj;
out vec3 vWorld;
out float vRho;
void main() {
    vec4 w = uModel * vec4(aPos, 1.0);
    vWorld = w.xyz;
    vRho = aRho;
    gl_Position = uViewProj * w;
}
)";

inline const char* kRingFS = R"(#version 330 core
in vec3 vWorld;
in float vRho;
out vec4 fragColor;
uniform sampler2D uRingTex;
uniform vec2 uRingRange;
uniform vec3 uCenter, uNormal;
uniform float uRadius;
void main() {
    vec4 t = texture(uRingTex, vec2((vRho - uRingRange.x) / (uRingRange.y - uRingRange.x), 0.5));
    vec3 L = normalize(-vWorld);
    float lit = 0.45 + 0.55 * sqrt(abs(dot(uNormal, L)));
    vec3 oc = uCenter - vWorld;           // the planet's shadow on the rings
    float along = dot(oc, L);
    if (along > 0.0 && dot(oc, oc) - along * along < uRadius * uRadius) lit *= 0.22;
    fragColor = vec4(t.rgb * lit, t.a);
}
)";

// ------------------------------------------------------------- overlays
// Screen-space quads: the dimming layer, the facts panel and text images.
inline const char* kQuadVS = R"(#version 330 core
layout(location = 0) in vec2 aPos;
uniform vec4 uRect;  // x, y, w, h in NDC
out vec2 vUV;
void main() {
    vUV = vec2(aPos.x, 1.0 - aPos.y);
    gl_Position = vec4(uRect.xy + aPos * uRect.zw, 0.0, 1.0);
}
)";

inline const char* kQuadFS = R"(#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uTex;
uniform int uUseTex;
uniform vec4 uColor;
void main() {
    if (uUseTex == 1) {
        vec4 t = texture(uTex, vUV);
        fragColor = vec4(t.rgb, t.a * uColor.a);
    } else {
        fragColor = uColor;
    }
}
)";

}  // namespace galaxis::shaders
