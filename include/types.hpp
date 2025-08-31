#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

struct RGBA {
    float r, g, b, a;
};

struct Mesh {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int vertex_count = 0;
};

// trying to structure it kind of like how most game engines do it
struct SceneObject {
    Mesh* mesh_data;
    
    glm::vec3 position;
    glm::vec3 velocity;
    float radius;

    glm::mat4 model_matrix = glm::mat4(1.0f);
};


struct UserState {
    const GLFWvidmode* mode;
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
    glm::vec3 target_position; // solely for lerp purposes
    
    float delta_time;
};
