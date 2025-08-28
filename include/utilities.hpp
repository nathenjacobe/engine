#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <glm/detail/qualifier.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

template <typename T>
T min(T a, T b) {
  return (a < b) ? a : b;
}

template <typename T>
T max(T a, T b) {
  return (a > b) ? a : b;
}

template <typename T>
T clamp(T value, T min_value, float max_value) {
    return max(min_value, min(value, max_value));
}

template <typename T>
T lerp(T a, T b, T t) {
  return a + t * (b - a);
}

template <typename T>
T lerp(const T& a, const T& b, float t) {
  return a + t * (b - a); 
}

glm::vec3 midpoint(glm::vec3 a, glm::vec3 b);

#endif // !UTILITIES_HPP
