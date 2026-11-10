/* glm/glm.hpp for the PS2 port: the part of GLM (https://github.com/g-truc/glm)
   that Daedalus uses, with GLM's conventions, so that the shared sources build
   unchanged: float vectors vec2/vec3/vec4 (x y z w, r g b a), column-major
   mat3/mat4 (m[column][row]), m * v and v * m, m * m, dot, cross, length,
   normalize, transpose, inverse and value_ptr (glm/gtc/type_ptr.hpp).
   Default construction gives zeros. */
#ifndef PS2_GLM_GLM_HPP
#define PS2_GLM_GLM_HPP

#include <math.h>

namespace glm {

#define PS2_GLM_VEC_OPS(V, N)                                                              \
  float &operator[](int i) { return (&x)[i]; }                                             \
  const float &operator[](int i) const { return (&x)[i]; }                                 \
  static constexpr int length() { return N; }                                              \
  V &operator+=(const V &o) { for (int i = 0; i < N; i++) (*this)[i] += o[i]; return *this; } \
  V &operator-=(const V &o) { for (int i = 0; i < N; i++) (*this)[i] -= o[i]; return *this; } \
  V &operator*=(const V &o) { for (int i = 0; i < N; i++) (*this)[i] *= o[i]; return *this; } \
  V &operator/=(const V &o) { for (int i = 0; i < N; i++) (*this)[i] /= o[i]; return *this; } \
  V &operator+=(float s) { for (int i = 0; i < N; i++) (*this)[i] += s; return *this; }     \
  V &operator-=(float s) { for (int i = 0; i < N; i++) (*this)[i] -= s; return *this; }     \
  V &operator*=(float s) { for (int i = 0; i < N; i++) (*this)[i] *= s; return *this; }     \
  V &operator/=(float s) { for (int i = 0; i < N; i++) (*this)[i] /= s; return *this; }

struct vec4;
struct vec3;

struct vec2 {
  union { struct { float x, y; }; struct { float r, g; }; struct { float s, t; }; };
  constexpr vec2() : x(0), y(0) {}
  constexpr explicit vec2(float v) : x(v), y(v) {}
  constexpr vec2(float a, float b) : x(a), y(b) {}
  inline explicit vec2(const vec3 &v);
  inline explicit vec2(const vec4 &v);
  PS2_GLM_VEC_OPS(vec2, 2)
};

struct vec3 {
  union { struct { float x, y, z; }; struct { float r, g, b; }; };
  constexpr vec3() : x(0), y(0), z(0) {}
  constexpr explicit vec3(float v) : x(v), y(v), z(v) {}
  constexpr vec3(float a, float b, float c) : x(a), y(b), z(c) {}
  constexpr vec3(const vec2 &v, float c) : x(v.x), y(v.y), z(c) {}
  inline explicit vec3(const vec4 &v);
  PS2_GLM_VEC_OPS(vec3, 3)
};

struct vec4 {
  union { struct { float x, y, z, w; }; struct { float r, g, b, a; }; };
  constexpr vec4() : x(0), y(0), z(0), w(0) {}
  constexpr explicit vec4(float v) : x(v), y(v), z(v), w(v) {}
  constexpr vec4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {}
  constexpr vec4(const vec3 &v, float d) : x(v.x), y(v.y), z(v.z), w(d) {}
  constexpr vec4(const vec2 &v, float c, float d) : x(v.x), y(v.y), z(c), w(d) {}
  PS2_GLM_VEC_OPS(vec4, 4)
};
#undef PS2_GLM_VEC_OPS

inline vec2::vec2(const vec3 &v) : x(v.x), y(v.y) {}
inline vec2::vec2(const vec4 &v) : x(v.x), y(v.y) {}
inline vec3::vec3(const vec4 &v) : x(v.x), y(v.y), z(v.z) {}

#define PS2_GLM_VEC_FREE(V, N)                                                             \
  inline V operator+(V a, const V &b) { return a += b; }                                   \
  inline V operator-(V a, const V &b) { return a -= b; }                                   \
  inline V operator*(V a, const V &b) { return a *= b; }                                   \
  inline V operator/(V a, const V &b) { return a /= b; }                                   \
  inline V operator+(V a, float s) { return a += s; }                                      \
  inline V operator-(V a, float s) { return a -= s; }                                      \
  inline V operator*(V a, float s) { return a *= s; }                                      \
  inline V operator/(V a, float s) { return a /= s; }                                      \
  inline V operator+(float s, V a) { return a += s; }                                      \
  inline V operator*(float s, V a) { return a *= s; }                                      \
  inline V operator-(float s, const V &a) { V r(s); return r -= a; }                       \
  inline V operator/(float s, const V &a) { V r(s); return r /= a; }                       \
  inline V operator-(V a) { for (int i = 0; i < N; i++) a[i] = -a[i]; return a; }          \
  inline V operator+(const V &a) { return a; }                                             \
  inline bool operator==(const V &a, const V &b) {                                         \
    for (int i = 0; i < N; i++) if (a[i] != b[i]) return false; return true; }             \
  inline bool operator!=(const V &a, const V &b) { return !(a == b); }                     \
  inline float dot(const V &a, const V &b) {                                               \
    float s = 0; for (int i = 0; i < N; i++) s += a[i] * b[i]; return s; }                 \
  inline float length(const V &a) { return sqrtf(dot(a, a)); }                             \
  inline V normalize(const V &a) { return a * (1.0f / sqrtf(dot(a, a))); }                 \
  inline V min(V a, const V &b) { for (int i = 0; i < N; i++) if (b[i] < a[i]) a[i] = b[i]; return a; } \
  inline V max(V a, const V &b) { for (int i = 0; i < N; i++) if (b[i] > a[i]) a[i] = b[i]; return a; } \
  inline V clamp(const V &a, float lo, float hi) { return min(max(a, V(lo)), V(hi)); }
PS2_GLM_VEC_FREE(vec2, 2)
PS2_GLM_VEC_FREE(vec3, 3)
PS2_GLM_VEC_FREE(vec4, 4)
#undef PS2_GLM_VEC_FREE

inline vec3 cross(const vec3 &a, const vec3 &b) {
  return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

struct mat4;

/* column-major: c[i] is column i, m[i][j] column i, row j (as GLM) */
struct mat3 {
  vec3 c[3];
  constexpr mat3() : mat3(1.0f) {}
  constexpr explicit mat3(float d) : c{vec3(d, 0, 0), vec3(0, d, 0), vec3(0, 0, d)} {}
  constexpr mat3(const vec3 &a, const vec3 &b, const vec3 &d) : c{a, b, d} {}
  inline explicit mat3(const mat4 &m);
  vec3 &operator[](int i) { return c[i]; }
  const vec3 &operator[](int i) const { return c[i]; }
  static constexpr int length() { return 3; }
};

struct mat4 {
  vec4 c[4];
  constexpr mat4() : mat4(1.0f) {}
  constexpr explicit mat4(float d)
      : c{vec4(d, 0, 0, 0), vec4(0, d, 0, 0), vec4(0, 0, d, 0), vec4(0, 0, 0, d)} {}
  constexpr mat4(const vec4 &a, const vec4 &b, const vec4 &d, const vec4 &e) : c{a, b, d, e} {}
  constexpr mat4(float m00, float m01, float m02, float m03, float m10, float m11, float m12,
                 float m13, float m20, float m21, float m22, float m23, float m30, float m31,
                 float m32, float m33)
      : c{vec4(m00, m01, m02, m03), vec4(m10, m11, m12, m13), vec4(m20, m21, m22, m23),
          vec4(m30, m31, m32, m33)} {}
  explicit mat4(const mat3 &m)
      : c{vec4(m[0], 0), vec4(m[1], 0), vec4(m[2], 0), vec4(0, 0, 0, 1)} {}
  vec4 &operator[](int i) { return c[i]; }
  const vec4 &operator[](int i) const { return c[i]; }
  static constexpr int length() { return 4; }
  inline mat4 &operator*=(const mat4 &o);
  mat4 &operator*=(float s) { for (auto &v : c) v *= s; return *this; }
};

inline mat3::mat3(const mat4 &m) : c{vec3(m[0]), vec3(m[1]), vec3(m[2])} {}

inline vec3 operator*(const mat3 &m, const vec3 &v) { return m[0] * v.x + m[1] * v.y + m[2] * v.z; }
inline vec4 operator*(const mat4 &m, const vec4 &v) {
  return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
}
inline vec3 operator*(const vec3 &v, const mat3 &m) { return vec3(dot(v, m[0]), dot(v, m[1]), dot(v, m[2])); }
inline vec4 operator*(const vec4 &v, const mat4 &m) {
  return vec4(dot(v, m[0]), dot(v, m[1]), dot(v, m[2]), dot(v, m[3]));
}
inline mat3 operator*(const mat3 &a, const mat3 &b) { return mat3(a * b[0], a * b[1], a * b[2]); }
inline mat4 operator*(const mat4 &a, const mat4 &b) {
  return mat4(a * b[0], a * b[1], a * b[2], a * b[3]);
}
inline mat4 &mat4::operator*=(const mat4 &o) { return *this = *this * o; }
inline mat4 operator*(const mat4 &a, float s) { return mat4(a[0] * s, a[1] * s, a[2] * s, a[3] * s); }
inline bool operator==(const mat4 &a, const mat4 &b) {
  return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}
inline bool operator!=(const mat4 &a, const mat4 &b) { return !(a == b); }

inline mat3 transpose(const mat3 &m) {
  mat3 r;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) r[i][j] = m[j][i];
  return r;
}
inline mat4 transpose(const mat4 &m) {
  mat4 r;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) r[i][j] = m[j][i];
  return r;
}

inline mat3 inverse(const mat3 &m) {
  /* the adjugate over the determinant */
  float a = m[0][0], b = m[1][0], c = m[2][0];
  float d = m[0][1], e = m[1][1], f = m[2][1];
  float g = m[0][2], h = m[1][2], i = m[2][2];
  float A = e * i - f * h, B = -(d * i - f * g), C = d * h - e * g;
  float det = a * A + b * B + c * C;
  float k = 1.0f / det;
  mat3 r;
  r[0][0] = A * k;  r[1][0] = -(b * i - c * h) * k;  r[2][0] = (b * f - c * e) * k;
  r[0][1] = B * k;  r[1][1] = (a * i - c * g) * k;   r[2][1] = -(a * f - c * d) * k;
  r[0][2] = C * k;  r[1][2] = -(a * h - b * g) * k;  r[2][2] = (a * e - b * d) * k;
  return r;
}

inline mat4 inverse(const mat4 &m) {
  /* cofactors (as GLM's compute_inverse for mat4) */
  const float *a = &m[0][0];
  float inv[16];
  inv[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
  inv[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
  inv[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
  inv[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
  inv[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
  inv[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
  inv[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
  inv[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
  inv[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15] + a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
  inv[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15] - a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
  inv[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15] + a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
  inv[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14] - a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
  inv[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11] - a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
  inv[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11] + a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
  inv[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11] - a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
  inv[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10] + a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];
  float det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
  float k = 1.0f / det;
  mat4 r;
  float *o = &r[0][0];
  for (int i = 0; i < 16; i++) o[i] = inv[i] * k;
  return r;
}

} // namespace glm

#endif
