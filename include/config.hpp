#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "./types.hpp"

#include <cstddef>
#include <glm/glm.hpp>

#include <filesystem>

// #include <string>
// #include <array>
#include <vector>
namespace CONFIG {

  // FPS 

  constexpr bool UNCAPPED_FRAMES = false;
  constexpr bool OUT_FPS = false;
 

  // USER INPUT

  constexpr float FOV_DEGREES = 120.0f;
  constexpr float FOV_MAX = 180.0f;
  constexpr float FOV_MIN = 1.0f;
  constexpr float FOV_SCROLL_SPEED = 2.0f;

  constexpr int MIN_FRAMES_PER_CURSOR_TOGGLE = 30;  

  constexpr float CAMERA_MOVEMENT_SPEED = 15.0f;
  constexpr float CAMERA_LERP_RATE = 0.8f;
  constexpr float MOUSE_SENSITIVITY = 0.15f;

  // PHYSICS

  const glm::vec3 GRAVITY_VECTOR = {0, -0.04, 0};

  constexpr float RAYCAST_STEP = 0.5;
  constexpr int RAYCAST_MAX_STEP = 100;
  constexpr int RAYCAST_BINARY_MAX_OPERATIONS = 10;
  constexpr float COLLISION_RESOLUTION_PRECISION = 0.01;


  // WINDOWING

  // unused, swapped for glfwGetVideoMode()->width, ->height
  // constexpr unsigned short WINDOW_WIDTH = 800;
  // constexpr unsigned short WINDOW_HEIGHT = 600;

  constexpr const char* WINDOW_NAME = "renderator";

  constexpr RGBA WINDOW_COLOR = {0.1f, 0.1f, 0.1f, 1.0f};
  
  constexpr float CONSTANT_ATTENUATION = 1.0f;
  constexpr float LINEAR_ATTENUATION = 0.14f;
  constexpr float QUADRATIC_ATTENUATION = 0.07f;

  // SHADING

  const std::filesystem::path SHADER_PATH = "./include/shaders"; // relative to main elf file 

  // TEXTURES

  const std::filesystem::path WALL_TEXTURE_PATH = "./res/textures/wall.jpg";
  const std::filesystem::path GATO_TEXTURE_PATH = "./res/textures/gato.png";
  const std::filesystem::path BLUEPRINT_TEXTURE_PATH = "./res/textures/blueprint.png";
  const std::filesystem::path WHITE_TEXTURE_PATH = "./res/textures/white.jpg";

  // MATERIALS
    
  constexpr material_data MATERIAL_ID_MAP[] = 
  {
    // {shininess, {ambience}, {diffuse}, {specular}} of the form {float, {vec3}, {vec3}, {vec3}}
    {12.8, {0.19125, 0.0735, 0.0225}, {0.7038, 0.27048, 0.0828}, {0.256777, 0.137622, 0.086014}}, // id 0: copper
    {12.8, {1, 1, 1}, {0.7038, 0.27048, 0.0828}, {0.256777, 0.137622, 0.086014}}, // id 1: unnecessarily clean copper
  };

  // MESHES

  const std::filesystem::path FBX_LAMP_PATH = "./res/meshes/lamp.fbx";
  const std::filesystem::path FBX_ICOSPHERE_PATH = "./res/meshes/icosphere.fbx"; 

  // MESH DATA

  constexpr unsigned short VERTEX_LENGTH = 8; // number of values per vertex ( = per line)

  // EXAMPLE CUBE DATA

  constexpr std::size_t CUBE_VERTEX_COUNT = 36; // number of vertices
  const std::vector<float> CUBE_VERTICES {
    // pos:vec3, normal:vec3, texture:vec2 
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,  
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, 
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,  
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, 

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
  
  };


  constexpr unsigned short NUM_CUBE_POSITIONS = 10;
  const glm::vec3 CUBE_POSITIONS[] = {
      glm::vec3( 0.0f,  0.0f,  0.0f),
      glm::vec3( 2.0f,  5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f),
      glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3( 2.4f, -0.4f, -3.5f),
      glm::vec3(-1.7f,  3.0f, -7.5f),
      glm::vec3( 1.3f, -2.0f, -2.5f),
      glm::vec3( 1.5f,  2.0f, -2.5f),
      glm::vec3( 1.5f,  0.2f, -1.5f),
      glm::vec3(-1.3f,  1.0f, -1.5f)
  };

}

#endif // !CONFIG_HPP

