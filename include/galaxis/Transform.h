// Transform.h - CP2 transformation matrices, written from scratch.
//
//  Mat3: 2-D homogeneous transformations (points are column vectors [x y 1]^T)
//  Mat4: 3-D homogeneous transformations (points are column vectors [x y z 1]^T)
//
// Matrices are stored column-major (m[col][row]) so a Mat4 can be handed to
// OpenGL directly in CP3. Composite transformations are built by
// multiplication; the right-most matrix is applied first:
//     M = T * R * S   means   scale, then rotate, then translate.
#pragma once

#include <array>
#include <vector>

#include "galaxis/Math.h"

namespace galaxis {

// ================================================================ 2-D
struct Mat3 {
    double m[3][3];  // m[col][row]

    static Mat3 identity();
    double& at(int row, int col) { return m[col][row]; }
    double at(int row, int col) const { return m[col][row]; }
};

Mat3 operator*(const Mat3& a, const Mat3& b);
Point operator*(const Mat3& a, Point p);  // p treated as [x y 1]

Mat3 translate2D(double tx, double ty);
Mat3 rotate2D(double angle);                       // about the origin, anticlockwise in y-up coordinates
Mat3 scale2D(double sx, double sy);                // about the origin
Mat3 reflectX();                                   // mirror in the x-axis  (y -> -y)
Mat3 reflectY();                                   // mirror in the y-axis  (x -> -x)
Mat3 reflectLine(double angle);                    // mirror in a line through the origin at `angle`
Mat3 shear2D(double shx, double shy);              // x' = x + shx*y,  y' = y + shy*x
Mat3 rotateAbout(Point pivot, double angle);       // T(p) * R * T(-p)
Mat3 scaleAbout(Point pivot, double sx, double sy);// T(p) * S * T(-p)

std::vector<Point> transform(const Mat3& m, const std::vector<Point>& poly);

// ================================================================ 3-D
struct Mat4 {
    double m[4][4];  // m[col][row]

    static Mat4 identity();
    double& at(int row, int col) { return m[col][row]; }
    double at(int row, int col) const { return m[col][row]; }
    std::array<float, 16> toFloat() const;  // column-major floats for glUniformMatrix4fv
};

Mat4 operator*(const Mat4& a, const Mat4& b);
Vec4 operator*(const Mat4& a, Vec4 v);
Vec3 transformPoint(const Mat4& a, Vec3 p);      // w = 1, then divides by w
Vec3 transformDirection(const Mat4& a, Vec3 d);  // w = 0 (ignores translation)

Mat4 translate3D(Vec3 t);
Mat4 scale3D(Vec3 s);
Mat4 rotateX(double angle);
Mat4 rotateY(double angle);
Mat4 rotateZ(double angle);
Mat4 rotateAxis(Vec3 axis, double angle);  // Rodrigues' rotation formula
Mat4 transpose(const Mat4& a);
Mat4 inverseRigid(const Mat4& a);          // inverse of rotation + translation (view matrices)

// Viewing pipeline
//   world --(view)--> eye --(projection)--> clip --(/w)--> NDC --(viewport)--> screen
Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up);
Mat4 perspective(double fovY, double aspect, double zNear, double zFar);
Mat4 orthographic(double left, double right, double bottom, double top, double zNear, double zFar);
Mat4 viewport(double width, double height);  // NDC [-1,1] -> pixels, y flipped (screen y grows down)

}  // namespace galaxis
