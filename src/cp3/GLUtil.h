// GLUtil.h - small OpenGL 3.3 helpers: shaders, buffers, textures.
#pragma once

#include <glad/gl.h>

#include <string>
#include <vector>

#include "galaxis/Canvas.h"
#include "galaxis/Math.h"
#include "galaxis/Transform.h"

namespace galaxis::gl {

class Shader {
public:
    bool build(const char* vertexSrc, const char* fragmentSrc, std::string& error);
    void use() const { glUseProgram(id_); }
    GLint loc(const char* name) const { return glGetUniformLocation(id_, name); }

    void set(const char* n, float v) const { glUniform1f(loc(n), v); }
    void set(const char* n, int v) const { glUniform1i(loc(n), v); }
    void set(const char* n, Vec3 v) const {
        glUniform3f(loc(n), static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
    }
    void set(const char* n, Color c, float a = 1.0f) const { glUniform4f(loc(n), c.r / 255.f, c.g / 255.f, c.b / 255.f, a); }
    void set(const char* n, const Mat4& m) const {
        const auto f = m.toFloat();
        glUniformMatrix4fv(loc(n), 1, GL_FALSE, f.data());
    }
    void set2(const char* n, float x, float y) const { glUniform2f(loc(n), x, y); }
    void set4(const char* n, float x, float y, float z, float w) const { glUniform4f(loc(n), x, y, z, w); }

private:
    GLuint id_ = 0;
};

// A vertex array with one interleaved float buffer.
struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei count = 0;  // indices (if ebo) or vertices
    GLenum mode = GL_TRIANGLES;

    // `layout` lists the float count of each attribute in order, e.g. {3,3,2}.
    void upload(const std::vector<float>& data, const std::vector<int>& layout, const std::vector<unsigned>& indices,
                GLenum drawMode, GLenum usage = GL_STATIC_DRAW);
    void update(const std::vector<float>& data);  // re-upload (dynamic meshes)
    void draw() const;
    void drawRange(GLint first, GLsizei n) const;  // non-indexed sub-range

private:
    int stride_ = 0;
};

GLuint makeTexture(int width, int height, const std::vector<unsigned char>& rgba, bool mipmaps, bool repeatS);

// Converts a Canvas into an RGBA image whose alpha is recovered with the
// "difference matte": the same artwork drawn once on black and once on white.
// a = 1 - (white - black), colour = black / a.
std::vector<unsigned char> matteToRGBA(const Canvas& onBlack, const Canvas& onWhite);

}  // namespace galaxis::gl
