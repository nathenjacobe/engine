
// using OPENGL 4.6 -> GLSL version 460 core

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#include "../include/ufbx.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

// #include <unordered_map>
// #include <array>
#include <vector>
#include <string>

#include <cstdio>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../include/config.hpp"
#include "../include/types.hpp"
#include "../include/utilities.hpp"

// debug
void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

// shading
GLenum getShaderType(const std::string& file_extension);
std::string stringifyShaderSource(const std::string& file_path);
unsigned int compileShader(const std::string& source, GLenum shader_type);
unsigned int createShaderProgram(const std::string& shader_folder_path);

void setBool(unsigned int shader_program, const std::string& name, bool value);
void setInt(unsigned int shader_program, const std::string& name, int value);
void setFloat(unsigned int shader_program, const std::string& name, float value);
void setMat4(unsigned int shader_program, const std::string& name, glm::mat4 value);
void setVec3(unsigned int shader_program, const std::string& name, glm::vec3 value);

// texturing
unsigned int loadTexture(const char *texture_path);

// materials

void bindMaterial(unsigned int shader_program, unsigned int material_id);


// mesh functions

std::vector<float> readFBXFile(const std::string& file_path);

mesh* generateMesh(const std::vector<float>& vertices, const unsigned int material_id);


// PHYSICS:

void translate(mesh* target_mesh, glm::vec3 factor);

// collision theory

bool isPointInBoundingBox(glm::vec3 point, bounding_box& box);

// raycasting:
raycast_result raycast(glm::vec3 origin, glm::vec3 direction, const std::vector<bounding_box>& boxes);
void predictVertexLocations(mesh* target_mesh, const std::vector<bounding_box>& boxes);

bool checkSphereAABBCollision(glm::vec3 sphere_center, float radius, glm::vec3 aabb_center, glm::vec3 half_widths);
void resolveCollision(mesh* target_mesh, float radius, glm::vec3 aabb_center, glm::vec3 half_widths);

// general physics update;

void updateSinglePhysics(mesh* target_mesh);
void updatePhysics(std::vector<mesh*> active_meshes, float delta_time);

// windowing and inputs
userState* generateUser(void);

GLFWwindow* createWindow(userState* user);
void updateColor(RGBA color);
void framebufferSizeCallback([[maybe_unused]] GLFWwindow* window, int width, int height);

void mouseCallback(GLFWwindow* window, double pos_x_in, double pos_y_in);
void scrollCallback(GLFWwindow* window, double offset_x, double offset_y);
void processInput(GLFWwindow* window, userState *user);
glm::vec3 determineFront(float yaw, float pitch);

void GLAPIENTRY messageCallback([[maybe_unused]] GLenum source, GLenum type, [[maybe_unused]] GLuint id,
    GLenum severity, [[maybe_unused]] GLsizei length, const GLchar* message, [[maybe_unused]] const void* userParam) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), 
        type, severity, message);
}


// SHADING


// This entire shebang is just in case i make more shaders. This just makes it a little easier to load all the shaders if / when i make them
// for now i'm only doing a vert and frag shader so getShaderType doesn't have all the enums converted.

GLenum getShaderType(const std::string& file_extension) {
  if (file_extension == ".vert") return GL_VERTEX_SHADER;
  if (file_extension == ".frag") return GL_FRAGMENT_SHADER;
  // ..  more to be added later if or when i make them
  std::cout << "Invalid extension type: " << file_extension << "\n";
  return 0;
}

std::string stringifyShaderSource(const std::string& file_path) {
  std::ifstream shader_file(file_path);
  
  if (!shader_file.is_open()) {
    std::cout << "Failed to open shader file at: " + file_path << "\n";
    return "";
  }

  // generate stringstream buffer to read the file into, and return it 
  std::stringstream buffer;
  buffer << shader_file.rdbuf();
  return buffer.str(); // convert to normal string
};

unsigned int compileShader(const std::string& shader_source, GLenum shader_type) {
  unsigned int shader = glCreateShader(shader_type);
  const char *shader_c_str = shader_source.c_str(); // convert to c_str so it is null-terminated
  glShaderSource(shader, 1, &shader_c_str, nullptr);
  glCompileShader(shader);

  // debug
  int success; // should be a GLint (and GLuint above) according to docs but nobody really cares
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, nullptr, infoLog); 
    std::cout << "failed to compile:\n" << infoLog << "\n";
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

unsigned int createShaderProgram(const std::string& shader_folder_path) {
  std::vector<unsigned int> shaders; // generate vector of shaders in shader_folder_path
  unsigned int shader_program = glCreateProgram();

  for (const auto& entry : std::filesystem::directory_iterator(shader_folder_path)) {
    // maybe one of the most niche set of functions to ever exist
    if (!entry.is_regular_file()) continue;
    
    std::string file_path = entry.path().string();
    std::string extension = entry.path().extension().string();
    
    GLenum shader_type = getShaderType(extension);
    std::string shader_source = stringifyShaderSource(file_path);
    int shader = compileShader(shader_source, shader_type);
    if (!shader) {
      std::cout << "Shader failed to load: " << file_path << "\n";
      continue;
    }

    shaders.push_back(shader);
  }
  
  for (unsigned int shader_index = 0; shader_index < shaders.size(); shader_index++) {
    unsigned int shader = shaders[shader_index];
    glAttachShader(shader_program, shader);
    glDeleteShader(shader);
  }

  glLinkProgram(shader_program);

  int success;
  char infoLog[512];
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, nullptr, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << "\n";
  }

  return shader_program;
}

void setBool(const unsigned int shader_program, const std::string& name, const bool value) {
  glUniform1i(glGetUniformLocation(shader_program, name.c_str()), (int)value);
}

void setInt(const unsigned int shader_program, const std::string& name, const int value) {
  glUniform1i(glGetUniformLocation(shader_program, name.c_str()), value);
}

void setFloat(const unsigned int shader_program, const std::string& name, const float value) {
  glUniform1f(glGetUniformLocation(shader_program, name.c_str()), value);
}

void setMat4(const unsigned int shader_program, const std::string& name, const glm::mat4 value) {
  glUniformMatrix4fv(glGetUniformLocation(shader_program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void setVec3(const unsigned int shader_program, const std::string& name, const glm::vec3 value) {
  glUniform3fv(glGetUniformLocation(shader_program, name.c_str()), 1, glm::value_ptr(value));
}

// TEXTURING


unsigned int loadTexture(const std::filesystem::path& texture_path, bool upside_down) {
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int width, height, channels;

  if (upside_down) {
    stbi_set_flip_vertically_on_load(true);
  }

  unsigned char *data = stbi_load(texture_path.string().c_str(), &width, &height, &channels, 0);
  

  if (data) {
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    //TODO: resolve to a default texture maybe?
    std::cout << "Failed to load texture at: " << texture_path << stbi_failure_reason() << "\n";
  }
  stbi_image_free(data);
  return texture;
}

// materials

void bindMaterial(unsigned int shader_program, unsigned int material_id) {
  const material_data* material = &CONFIG::MATERIAL_ID_MAP[material_id]; 
  setFloat(shader_program, "material.shininess", material->shininess);
  setVec3(shader_program, "material.ambient", material->ambient);
  setVec3(shader_program, "material.diffuse", material->diffuse);
  setVec3(shader_program, "material.specular", material->specular);
}


// meshes

std::vector<float> readFBXFile(const std::string& file_path) {
  std::vector<float> mesh_data;

  ufbx_load_opts opts = { };
  ufbx_error error = { };
  ufbx_scene *scene = ufbx_load_file(file_path.c_str(), &opts, &error);

  if (!scene) return mesh_data;

  for (size_t i = 0; i < scene->meshes.count; i++) {
      ufbx_mesh *mesh = scene->meshes.data[i];
      
      for (ufbx_face face : mesh->faces) {
          std::vector<uint32_t> tri_indices(mesh->max_face_triangles * 3);
          
          uint32_t num_tris = ufbx_triangulate_face(
              tri_indices.data(), tri_indices.size(), mesh, face);
              
          for (size_t t = 0; t < num_tris * 3; t++) {
              uint32_t index = tri_indices[t];
              
              ufbx_vec3 position = mesh->vertex_position[index];
              ufbx_vec3 normal = mesh->vertex_normal[index];
              ufbx_vec2 uv = mesh->vertex_uv[index];
              
              mesh_data.push_back(static_cast<float>(position.x));
              mesh_data.push_back(static_cast<float>(position.y));
              mesh_data.push_back(static_cast<float>(position.z));
              
              mesh_data.push_back(static_cast<float>(normal.x));
              mesh_data.push_back(static_cast<float>(normal.y));
              mesh_data.push_back(static_cast<float>(normal.z));
              
              mesh_data.push_back(static_cast<float>(uv.x));
              mesh_data.push_back(static_cast<float>(uv.y));
          }
      }
  }

  ufbx_free_scene(scene);

  return mesh_data;
}

mesh* generateMesh(const std::vector<float>& vertices, const unsigned int material_id) {
  mesh* new_mesh = new mesh {
    0, {0, 0, 0}, vertices, 0, 0, material_id, {0, 0, 0}
  };

  glGenVertexArrays(1, &new_mesh->vao);
  glBindVertexArray(new_mesh->vao);

  glGenBuffers(1, &new_mesh->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, new_mesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * CONFIG::VERTEX_LENGTH * (vertices.size() / CONFIG::VERTEX_LENGTH), vertices.data(), GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, CONFIG::VERTEX_LENGTH * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, CONFIG::VERTEX_LENGTH * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, CONFIG::VERTEX_LENGTH * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);

  return new_mesh;
}

// PHYSICS SIMULATION

// GENERAL FUNCTIONS

void translate(mesh* target_mesh, glm::vec3 factor) {
  std::cout << "translated" << "\n";
  unsigned int vertex_count = target_mesh->vertices.size() / CONFIG::VERTEX_LENGTH;

  target_mesh->root_position += factor;
  for (unsigned long index = 0; index < vertex_count; index++) {
    target_mesh->vertices[index] += factor.x;
    target_mesh->vertices[index + 1] += factor.y;
    target_mesh->vertices[index + 2] += factor.z;
  }
  
}

// COLLISION DETECTION

bool isPointInBoundingBox(glm::vec3 point, bounding_box& box) {
  return (
    (min(box.p1.x, box.p2.x) <= point.x && point.x <= max(box.p1.x, box.p2.x)) &&
    (min(box.p1.y, box.p2.y) <= point.y && point.y <= max(box.p1.y, box.p2.y)) &&
    (min(box.p1.z, box.p2.z) <= point.z && point.z <= max(box.p1.z, box.p2.z))
  );
}

// raycast functions

// primitive collision checks

bool checkSphereAABBCollision(glm::vec3 sphere_center, float radius, glm::vec3 aabb_center, glm::vec3 half_widths) {
  glm::vec3 clamped = {
    clamp(sphere_center.x, aabb_center.x - half_widths.x, aabb_center.x + half_widths.x),
    clamp(sphere_center.y, aabb_center.y - half_widths.y, aabb_center.y + half_widths.y),
    clamp(sphere_center.z, aabb_center.z - half_widths.z, aabb_center.z + half_widths.z)
  };

  float distance_squared = 
    (sphere_center.x - clamped.x) * (sphere_center.x - clamped.x) +
    (sphere_center.y - clamped.y) * (sphere_center.y - clamped.y) +
    (sphere_center.z - clamped.z) * (sphere_center.z - clamped.z);
    
  std::cout << distance_squared << "\n";

  return (distance_squared <= (radius * radius));

}


// primitive resolution

void resolveCollision(mesh* target_mesh, float radius, glm::vec3 aabb_center, glm::vec3 half_widths) {
  while (checkSphereAABBCollision(target_mesh->root_position, radius, aabb_center, half_widths)) {
    translate(target_mesh, target_mesh->velocity * -1.0f * CONFIG::COLLISION_RESOLUTION_PRECISION);
  }
}


// global physics update + render

void updateSinglePhysics(mesh* target_mesh) {
  translate(target_mesh, target_mesh->velocity);

  glBindBuffer(GL_ARRAY_BUFFER, target_mesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * target_mesh->vertices.size(), target_mesh->vertices.data(), GL_STATIC_DRAW);
}

void updatePhysics(std::vector<mesh*> active_meshes, float delta_time) {
  for (mesh* target_mesh : active_meshes) {
    if (!target_mesh->anchored) target_mesh->velocity += CONFIG::GRAVITY_VECTOR * delta_time;
    else target_mesh->velocity = {0, 0, 0};
    updateSinglePhysics(target_mesh);
  }
}


// WINDOWING

userState* generateUser(void) {


  return new userState {
    nullptr,
    90.0f, // fov
    
    true, // first mouse
    true, // lock_cursor
    CONFIG::MIN_FRAMES_PER_CURSOR_TOGGLE, // frames since cursor toggle

    -90.0f, // yaw
    0.0f, // pitch
    600 / 2.0f, // last x
    600 / 2.0f, // last y

    {0, 10, 8}, // camera pos
    {0, 0, 0}, // camera front
    {0, 1, 0}, // camera up
    {0, 10, 8}, // target pos
    0.0f
  };

}


GLFWwindow* createWindow(userState* user) {

  if (!glfwInit()) {
    std::cout << "Failed to init GLFW" << "\n";
    return nullptr;
  }

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  user->mode = mode;

  if (!monitor) {
    std::cout << "Failed to get primary monitor" << "\n";
    glfwTerminate();
    return nullptr;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  
  glfwWindowHint(GLFW_RED_BITS, user->mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, user->mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, user->mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, user->mode->refreshRate);


  GLFWwindow* window = glfwCreateWindow(user->mode->width, user->mode->height, CONFIG::WINDOW_NAME, monitor, nullptr);
  
  if (!window) {
    std::cout << "Failed to create GLFW Window" << "\n";
    glfwTerminate();
    return nullptr;
  }

  glfwSetWindowMonitor(
    window, 
    monitor,
    0, 0, 
    user->mode->width, 
    user->mode->height, 
    user->mode->refreshRate
  );

  glfwMakeContextCurrent(window);

  // 0 is uncapped
  // 1 is 60
  // 2 is 30
  // 3+ is 20
  // it's absolute value'd
  if (CONFIG::UNCAPPED_FRAMES) glfwSwapInterval(0);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  if (glfwRawMouseMotionSupported()) {
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }


  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetScrollCallback(window, scrollCallback);
 
  glfwSetWindowUserPointer(window, user);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialise GLAD" << "\n";
    return nullptr;
  }

  return window;
}

void updateColor(RGBA color) {
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void processInput(GLFWwindow* window, userState* user) {

  user->frames_since_cursor_toggle++;

  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  } else if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
    if (user->frames_since_cursor_toggle < CONFIG::MIN_FRAMES_PER_CURSOR_TOGGLE) return;
    user->lock_cursor = !user->lock_cursor;
    glfwSetInputMode(window, GLFW_CURSOR, user->lock_cursor ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    user->frames_since_cursor_toggle = 0; 
  }

  float camera_speed = CONFIG::CAMERA_MOVEMENT_SPEED * user->delta_time;
    
  glm::vec3 right = glm::normalize(glm::cross(user->camera_front, user->camera_up));
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) user->target_position += user->camera_front * camera_speed;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) user->target_position -= user->camera_front * camera_speed;
        
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) user->target_position -= right * camera_speed; 
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) user->target_position += right * camera_speed;

  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) user->target_position -= user->camera_up * camera_speed; 
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) user->target_position += user->camera_up * camera_speed; 

  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) user->target_position = lerp(user->target_position, user->camera_position,
                                                                                     CONFIG::CAMERA_LERP_RATE * 5 * user->delta_time);

  user->camera_position = lerp(user->camera_position, user->target_position, CONFIG::CAMERA_LERP_RATE * user->delta_time);
}

void framebufferSizeCallback([[maybe_unused]] GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

// INPUT

glm::vec3 determineFront(float yaw, float pitch) {
  return {
    cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
    sin(glm::radians(pitch)),
    sin(glm::radians(yaw)) * cos(glm::radians(pitch))
  };
}

void mouseCallback(GLFWwindow* window, double x_pos_in, double y_pos_in) {
  float x_pos = static_cast<float>(x_pos_in);
  float y_pos = static_cast<float>(y_pos_in);
  
  userState* user = static_cast<userState*>(glfwGetWindowUserPointer(window));

  if (user->first_mouse) {
    user->last_x = x_pos;
    user->last_y = y_pos;
    user->first_mouse = false;
  }

  // this is our mouse delta
  float x_offset = x_pos - user->last_x;
  float y_offset = user->last_y - y_pos;

  user->last_x = x_pos;
  user->last_y = y_pos;

  x_offset *= CONFIG::MOUSE_SENSITIVITY;
  y_offset *= CONFIG::MOUSE_SENSITIVITY;

  user->yaw += x_offset;
  user->pitch += y_offset;
  user->pitch = clamp(user->pitch, -89.0f, 89.0f);
  
  glm::vec3 front = determineFront(user->yaw, user->pitch);
  user->camera_front = glm::normalize(front);
}

void scrollCallback(GLFWwindow* window, [[maybe_unused]] double x_offset, double y_offset) {
  userState* user = static_cast<userState*>(glfwGetWindowUserPointer(window));
  user->fov = clamp(user->fov - float(y_offset) * CONFIG::FOV_SCROLL_SPEED, CONFIG::FOV_MIN, CONFIG::FOV_MAX);
}


// CORE LOGIC


int main() {

  RGBA window_color = CONFIG::WINDOW_COLOR;

  glm::vec3 light_position = {10.0f, -10.0f, 10.0f};
  glm::vec3 light_color = {0.983f, 0.963f, 0.872f};

  userState* user = generateUser();
  std::vector<mesh*> active_meshes;

  float current_frame = 0.0f;
  float last_frame = 0.0f;

  GLFWwindow* window = createWindow(user); 
  if (!window) {
    std::cout << "No window found." << "\n";
    return -1;
  }


  // enable culling
  glEnable(GL_DEPTH_TEST);  

  // enable debug
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(messageCallback, 0);
  

  unsigned int shader_program = createShaderProgram(CONFIG::SHADER_PATH); 
  glUseProgram(shader_program);

  // bind materials:
  bindMaterial(shader_program, 1);

  // add light attributes to shader
  setVec3(shader_program, "light_position", light_position);
  setVec3(shader_program, "light_color", light_color);

  // add camera view pos to shader
  setVec3(shader_program, "view_position", user->camera_position);

  // add attenuation factors to shader
  setFloat(shader_program, "constant_attenuation", CONFIG::CONSTANT_ATTENUATION);
  setFloat(shader_program, "linear_attenuation", CONFIG::LINEAR_ATTENUATION);
  setFloat(shader_program, "quadratic_attenuation", CONFIG::QUADRATIC_ATTENUATION);

  // load textures:

  unsigned int wall_texture = loadTexture(CONFIG::WALL_TEXTURE_PATH, false);
  unsigned int gato_texture = loadTexture(CONFIG::GATO_TEXTURE_PATH, true);
  unsigned int blueprint_texture = loadTexture(CONFIG::BLUEPRINT_TEXTURE_PATH, false);
  unsigned int white_texture = loadTexture(CONFIG::WHITE_TEXTURE_PATH, false);

  if (gato_texture == 0 || blueprint_texture == 0 || wall_texture == 0) {
    std::cout << "lol your texture(s) failed to load. you probably got the path wrong or something haha" << "\n";
    return -1;
  }

  // line 702 for gl_texturei enums

  setInt(shader_program, "primary_texture", 0);

  mesh* baseplate = generateMesh(CONFIG::CUBE_VERTICES, 1);
  baseplate->anchored = true;
  active_meshes.push_back(baseplate);
  
  glm::mat4 baseplate_model = glm::mat4(1.0f);
  baseplate_model = glm::scale(baseplate_model, {15, 0.1, 15});
  
  std::vector<float> icosphere_vertices = readFBXFile(CONFIG::FBX_ICOSPHERE_PATH); 
  mesh* icosphere = generateMesh(icosphere_vertices, 1);
  active_meshes.push_back(icosphere);

  icosphere->velocity = {0, -1, 0};

  glm::mat4 icosphere_model = glm::mat4(1.0f);
  icosphere_model = glm::translate(icosphere_model, {0, 10, 0});
  icosphere->root_position = {0, 10, 0};

  // main loop
  while (!glfwWindowShouldClose(window)) {
    std::cout << "started the loop" << "\n"; 
    // get user
    
    userState *user = static_cast<userState*>(glfwGetWindowUserPointer(window));

    // delta calculations
    current_frame = (float)glfwGetTime();
    user->delta_time = current_frame - last_frame;
    last_frame = current_frame;

    if (CONFIG::OUT_FPS) std::cout << "fps: " <<  1.0f / user->delta_time << "\n";

    processInput(window, user);
    
    updateColor(window_color);
    glUseProgram(shader_program);

    // lighting and camera matrices

    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(user->fov), (float)user->mode->width / (float)user->mode->height, 0.1f, 100.0f);
    view = glm::lookAt(user->camera_position, user->camera_position + user->camera_front, user->camera_up);

    setMat4(shader_program, "projection", projection);
    setMat4(shader_program, "view", view);
   
    setVec3(shader_program, "view_pos", user->camera_position);
    setVec3(shader_program, "light_position", light_position);
    setVec3(shader_program, "light_color", light_color);

    // baseplate

    glBindVertexArray(baseplate->vao);
    
    setMat4(shader_program, "model", baseplate_model);

    bindMaterial(shader_program, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blueprint_texture);

    glDrawArrays(GL_TRIANGLES, 0, CONFIG::CUBE_VERTEX_COUNT);

    // icosphere
    glBindVertexArray(icosphere->vao);

    setMat4(shader_program, "model", icosphere_model);  

    bindMaterial(shader_program, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, white_texture);

    glDrawArrays(GL_TRIANGLES, 0, icosphere_vertices.size() / CONFIG::VERTEX_LENGTH);

    updatePhysics(active_meshes, user->delta_time);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &baseplate->vao);
  glDeleteBuffers(1, &baseplate->vbo);

  glDeleteVertexArrays(1, &icosphere->vao);
  glDeleteBuffers(1, &icosphere->vbo);
  // glDeleteBuffers(1, &EBO);
  glDeleteProgram(shader_program);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

