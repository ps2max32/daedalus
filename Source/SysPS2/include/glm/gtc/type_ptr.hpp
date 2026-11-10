/* glm/gtc/type_ptr.hpp for the PS2 port (see glm/glm.hpp): value_ptr */
#ifndef PS2_GLM_TYPE_PTR_HPP
#define PS2_GLM_TYPE_PTR_HPP

#include <glm/glm.hpp>

namespace glm {
inline float *value_ptr(vec2 &v) { return &v.x; }
inline float *value_ptr(vec3 &v) { return &v.x; }
inline float *value_ptr(vec4 &v) { return &v.x; }
inline float *value_ptr(mat3 &m) { return &m[0].x; }
inline float *value_ptr(mat4 &m) { return &m[0].x; }
inline const float *value_ptr(const vec2 &v) { return &v.x; }
inline const float *value_ptr(const vec3 &v) { return &v.x; }
inline const float *value_ptr(const vec4 &v) { return &v.x; }
inline const float *value_ptr(const mat3 &m) { return &m[0].x; }
inline const float *value_ptr(const mat4 &m) { return &m[0].x; }
} // namespace glm

#endif
