// Transform.cpp - 2-D and 3-D homogeneous transformation matrices.
#include "galaxis/Transform.h"

#include <cmath>

namespace galaxis {

// ================================================================ 2-D
Mat3 Mat3::identity() {
    Mat3 r{};
    for (int i = 0; i < 3; ++i) r.m[i][i] = 1.0;
    return r;
}

Mat3 operator*(const Mat3& a, const Mat3& b) {
    Mat3 r{};
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col) {
            double s = 0;
            for (int k = 0; k < 3; ++k) s += a.at(row, k) * b.at(k, col);
            r.at(row, col) = s;
        }
    return r;
}

Point operator*(const Mat3& a, Point p) {
    const double x = a.at(0, 0) * p.x + a.at(0, 1) * p.y + a.at(0, 2);
    const double y = a.at(1, 0) * p.x + a.at(1, 1) * p.y + a.at(1, 2);
    const double w = a.at(2, 0) * p.x + a.at(2, 1) * p.y + a.at(2, 2);
    return {x / w, y / w};
}

Mat3 translate2D(double tx, double ty) {
    Mat3 r = Mat3::identity();
    r.at(0, 2) = tx;
    r.at(1, 2) = ty;
    return r;
}

Mat3 rotate2D(double a) {
    Mat3 r = Mat3::identity();
    const double c = std::cos(a), s = std::sin(a);
    r.at(0, 0) = c;
    r.at(0, 1) = -s;
    r.at(1, 0) = s;
    r.at(1, 1) = c;
    return r;
}

Mat3 scale2D(double sx, double sy) {
    Mat3 r = Mat3::identity();
    r.at(0, 0) = sx;
    r.at(1, 1) = sy;
    return r;
}

Mat3 reflectX() { return scale2D(1, -1); }
Mat3 reflectY() { return scale2D(-1, 1); }

Mat3 reflectLine(double angle) {
    // Rotate the line onto the x-axis, mirror, rotate back.
    return rotate2D(angle) * reflectX() * rotate2D(-angle);
}

Mat3 shear2D(double shx, double shy) {
    Mat3 r = Mat3::identity();
    r.at(0, 1) = shx;
    r.at(1, 0) = shy;
    return r;
}

Mat3 rotateAbout(Point p, double angle) { return translate2D(p.x, p.y) * rotate2D(angle) * translate2D(-p.x, -p.y); }

Mat3 scaleAbout(Point p, double sx, double sy) {
    return translate2D(p.x, p.y) * scale2D(sx, sy) * translate2D(-p.x, -p.y);
}

std::vector<Point> transform(const Mat3& m, const std::vector<Point>& poly) {
    std::vector<Point> out;
    out.reserve(poly.size());
    for (const Point& p : poly) out.push_back(m * p);
    return out;
}

// ================================================================ 3-D
Mat4 Mat4::identity() {
    Mat4 r{};
    for (int i = 0; i < 4; ++i) r.m[i][i] = 1.0;
    return r;
}

std::array<float, 16> Mat4::toFloat() const {
    std::array<float, 16> f{};
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row) f[static_cast<std::size_t>(col * 4 + row)] = static_cast<float>(m[col][row]);
    return f;
}

Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col) {
            double s = 0;
            for (int k = 0; k < 4; ++k) s += a.at(row, k) * b.at(k, col);
            r.at(row, col) = s;
        }
    return r;
}

Vec4 operator*(const Mat4& a, Vec4 v) {
    const double in[4] = {v.x, v.y, v.z, v.w};
    double out[4];
    for (int row = 0; row < 4; ++row) {
        out[row] = 0;
        for (int k = 0; k < 4; ++k) out[row] += a.at(row, k) * in[k];
    }
    return {out[0], out[1], out[2], out[3]};
}

Vec3 transformPoint(const Mat4& a, Vec3 p) {
    const Vec4 r = a * Vec4{p.x, p.y, p.z, 1.0};
    return {r.x / r.w, r.y / r.w, r.z / r.w};
}

Vec3 transformDirection(const Mat4& a, Vec3 d) {
    const Vec4 r = a * Vec4{d.x, d.y, d.z, 0.0};
    return {r.x, r.y, r.z};
}

Mat4 translate3D(Vec3 t) {
    Mat4 r = Mat4::identity();
    r.at(0, 3) = t.x;
    r.at(1, 3) = t.y;
    r.at(2, 3) = t.z;
    return r;
}

Mat4 scale3D(Vec3 s) {
    Mat4 r = Mat4::identity();
    r.at(0, 0) = s.x;
    r.at(1, 1) = s.y;
    r.at(2, 2) = s.z;
    return r;
}

Mat4 rotateX(double a) {
    Mat4 r = Mat4::identity();
    const double c = std::cos(a), s = std::sin(a);
    r.at(1, 1) = c;
    r.at(1, 2) = -s;
    r.at(2, 1) = s;
    r.at(2, 2) = c;
    return r;
}

Mat4 rotateY(double a) {
    Mat4 r = Mat4::identity();
    const double c = std::cos(a), s = std::sin(a);
    r.at(0, 0) = c;
    r.at(0, 2) = s;
    r.at(2, 0) = -s;
    r.at(2, 2) = c;
    return r;
}

Mat4 rotateZ(double a) {
    Mat4 r = Mat4::identity();
    const double c = std::cos(a), s = std::sin(a);
    r.at(0, 0) = c;
    r.at(0, 1) = -s;
    r.at(1, 0) = s;
    r.at(1, 1) = c;
    return r;
}

Mat4 rotateAxis(Vec3 axis, double a) {
    const Vec3 k = normalize(axis);
    const double c = std::cos(a), s = std::sin(a), t = 1 - c;
    Mat4 r = Mat4::identity();
    r.at(0, 0) = t * k.x * k.x + c;
    r.at(0, 1) = t * k.x * k.y - s * k.z;
    r.at(0, 2) = t * k.x * k.z + s * k.y;
    r.at(1, 0) = t * k.x * k.y + s * k.z;
    r.at(1, 1) = t * k.y * k.y + c;
    r.at(1, 2) = t * k.y * k.z - s * k.x;
    r.at(2, 0) = t * k.x * k.z - s * k.y;
    r.at(2, 1) = t * k.y * k.z + s * k.x;
    r.at(2, 2) = t * k.z * k.z + c;
    return r;
}

Mat4 transpose(const Mat4& a) {
    Mat4 r{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col) r.at(row, col) = a.at(col, row);
    return r;
}

Mat4 inverseRigid(const Mat4& a) {
    // [R t]^-1 = [R^T  -R^T t]
    Mat4 r = Mat4::identity();
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col) r.at(row, col) = a.at(col, row);
    const Vec3 t{a.at(0, 3), a.at(1, 3), a.at(2, 3)};
    const Vec3 rt = transformDirection(r, t);
    r.at(0, 3) = -rt.x;
    r.at(1, 3) = -rt.y;
    r.at(2, 3) = -rt.z;
    return r;
}

Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    // Camera basis: f looks forward, s to the right, u up. The view matrix
    // rotates the world into this basis and moves the eye to the origin.
    const Vec3 f = normalize(target - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);
    Mat4 r = Mat4::identity();
    r.at(0, 0) = s.x;
    r.at(0, 1) = s.y;
    r.at(0, 2) = s.z;
    r.at(1, 0) = u.x;
    r.at(1, 1) = u.y;
    r.at(1, 2) = u.z;
    r.at(2, 0) = -f.x;
    r.at(2, 1) = -f.y;
    r.at(2, 2) = -f.z;
    r.at(0, 3) = -dot(s, eye);
    r.at(1, 3) = -dot(u, eye);
    r.at(2, 3) = dot(f, eye);
    return r;
}

Mat4 perspective(double fovY, double aspect, double n, double f) {
    const double t = 1.0 / std::tan(fovY / 2);
    Mat4 r{};
    r.at(0, 0) = t / aspect;
    r.at(1, 1) = t;
    r.at(2, 2) = (f + n) / (n - f);
    r.at(2, 3) = 2 * f * n / (n - f);
    r.at(3, 2) = -1;
    return r;
}

Mat4 orthographic(double l, double rgt, double b, double t, double n, double f) {
    Mat4 r = Mat4::identity();
    r.at(0, 0) = 2 / (rgt - l);
    r.at(1, 1) = 2 / (t - b);
    r.at(2, 2) = -2 / (f - n);
    r.at(0, 3) = -(rgt + l) / (rgt - l);
    r.at(1, 3) = -(t + b) / (t - b);
    r.at(2, 3) = -(f + n) / (f - n);
    return r;
}

Mat4 viewport(double w, double h) {
    Mat4 r = Mat4::identity();
    r.at(0, 0) = w / 2;
    r.at(0, 3) = w / 2;
    r.at(1, 1) = -h / 2;  // NDC +y is up, screen +y is down
    r.at(1, 3) = h / 2;
    return r;
}

}  // namespace galaxis
