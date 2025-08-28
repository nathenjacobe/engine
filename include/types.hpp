#ifndef TYPES_HPP
#define TYPES_HPP

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>

#include <glm/detail/qualifier.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

#include <vector>
 
// #include <array>

struct RGBA {
  float r, g, b, a;
};

struct triangle {
  float vertices[9];
  float colors[9];
  float textures[4];
};

struct userState {
  const GLFWvidmode *mode;

  float fov;

  bool first_mouse;
  bool lock_cursor;
  int frames_since_cursor_toggle;
  float yaw;
  float pitch;
  float last_x;
  float last_y;
  
  glm::vec3 camera_position;
  glm::vec3 camera_front;
  glm::vec3 camera_up;

  glm::vec3 target_position;

  float delta_time;
};

struct mesh {
  bool anchored;
  glm::vec3 root_position;
  std::vector<float> vertices;
  unsigned int vao;
  unsigned int vbo;
  unsigned int material_id;
  glm::vec3 velocity;
};

struct texture_data {
  unsigned int id;
  int width;
  int height;
  int channels;
};

struct material_data {
  float shininess;
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
};

struct bounding_box {
  glm::vec3 p1;
  glm::vec3 p2;
};

struct raycast_result {
  bool success;
  unsigned int box_index;
  glm::vec3 location;
};

#endif // !TYPES_HPP
