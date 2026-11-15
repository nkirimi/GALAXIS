// GLUtil.cpp
#define GLAD_GL_IMPLEMENTATION  // the header-only GLAD loader lives in this file
#include "GLUtil.h"

#include <algorithm>

namespace galaxis::gl {

namespace {

GLuint compile(GLenum type, const char* src, std::string& error) {
    const GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, sizeof log, nullptr, log);
        error += std::string(type == GL_VERTEX_SHADER ? "vertex" : "fragment") + " shader: " + log;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

}  // namespace

bool Shader::build(const char* vs, const char* fs, std::string& error) {
    const GLuint v = compile(GL_VERTEX_SHADER, vs, error);
    const GLuint f = compile(GL_FRAGMENT_SHADER, fs, error);
    if (!v || !f) return false;
    id_ = glCreateProgram();
    glAttachShader(id_, v);
    glAttachShader(id_, f);
    glLinkProgram(id_);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(id_, sizeof log, nullptr, log);
        error += std::string("link: ") + log;
        return false;
    }
    return true;
}

void Mesh::upload(const std::vector<float>& data, const std::vector<int>& layout, const std::vector<unsigned>& indices,
                  GLenum drawMode, GLenum usage) {
    mode = drawMode;
    if (!vao) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), usage);
    stride_ = 0;
    for (int n : layout) stride_ += n;
    std::size_t offset = 0;
    for (std::size_t i = 0; i < layout.size(); ++i) {
        glEnableVertexAttribArray(static_cast<GLuint>(i));
        glVertexAttribPointer(static_cast<GLuint>(i), layout[i], GL_FLOAT, GL_FALSE, stride_ * static_cast<GLsizei>(sizeof(float)),
                              reinterpret_cast<const void*>(offset * sizeof(float)));
        offset += static_cast<std::size_t>(layout[i]);
    }
    if (!indices.empty()) {
        if (!ebo) glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned)), indices.data(),
                     GL_STATIC_DRAW);
        count = static_cast<GLsizei>(indices.size());
    } else {
        count = static_cast<GLsizei>(data.size() / static_cast<std::size_t>(stride_));
    }
    glBindVertexArray(0);
}

void Mesh::update(const std::vector<float>& data) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STREAM_DRAW);
    if (!ebo) count = static_cast<GLsizei>(data.size() / static_cast<std::size_t>(stride_));
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    if (ebo)
        glDrawElements(mode, count, GL_UNSIGNED_INT, nullptr);
    else
        glDrawArrays(mode, 0, count);
}

void Mesh::drawRange(GLint first, GLsizei n) const {
    glBindVertexArray(vao);
    glDrawArrays(mode, first, n);
}

GLuint makeTexture(int w, int h, const std::vector<unsigned char>& rgba, bool mipmaps, bool repeatS) {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeatS ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    return t;
}

std::vector<unsigned char> matteToRGBA(const Canvas& b, const Canvas& w) {
    std::vector<unsigned char> out(static_cast<std::size_t>(b.width()) * b.height() * 4);
    for (int y = 0; y < b.height(); ++y)
        for (int x = 0; x < b.width(); ++x) {
            const Color cb = b.getPixel(x, y), cw = w.getPixel(x, y);
            const double diff = ((cw.r - cb.r) + (cw.g - cb.g) + (cw.b - cb.b)) / (3.0 * 255.0);
            const double a = std::clamp(1.0 - diff, 0.0, 1.0);
            const std::size_t k = (static_cast<std::size_t>(y) * b.width() + x) * 4;
            auto un = [&](int v) { return static_cast<unsigned char>(a > 0.004 ? std::clamp(v / a, 0.0, 255.0) : 0); };
            out[k + 0] = un(cb.r);
            out[k + 1] = un(cb.g);
            out[k + 2] = un(cb.b);
            out[k + 3] = static_cast<unsigned char>(a * 255 + 0.5);
        }
    return out;
}

}  // namespace galaxis::gl
