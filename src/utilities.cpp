
#include "utilities.hpp"
#include <glm/detail/qualifier.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

glm::vec3 midpoint(glm::vec3 a, glm::vec3 b) {
  return 0.5f * (a + b);
}
