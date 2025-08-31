// using OPENGL 4.6 -> GLSL version 460 core

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"
#include "../include/ufbx.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>

#include "../include/config.hpp"
#include "../include/types.hpp"

void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

GLenum getShaderType(const std::string& file_extension);
std::string stringifyShaderSource(const std::string& file_path);
unsigned int compileShader(const std::string& source, GLenum shader_type);
unsigned int createShaderProgram(const std::string& shader_folder_path);
void setInt(unsigned int shader_program, const std::string& name, int value);
void setBool(unsigned int shader_program, const std::string& name, bool value);
void setFloat(unsigned int shader_program, const std::string& name, float value);
void setMat4(unsigned int shader_program, const std::string& name, const glm::mat4& value);
void setVec3(unsigned int shader_program, const std::string& name, const glm::vec3& value);

unsigned int loadTexture(const std::filesystem::path& texture_path, bool upside_down);

std::vector<float> readFBXFile(const std::string& file_path);
Mesh* generateMesh(const std::vector<float>& vertices);

GLFWwindow* createWindow(UserState* user);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double pos_x_in, double pos_y_in);
void scrollCallback(GLFWwindow* window, double offset_x, double offset_y);
void processInput(GLFWwindow* window, UserState *user);
glm::vec3 determineFront(float yaw, float pitch);


float randomFloat(float min, float max) {
    // who named it mt19937?? cool name either way haha
    static std::mt19937 generator(std::random_device{}());
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(generator);
}

SceneObject createIcosphere(Mesh* sphere_mesh, float radius) {
    SceneObject sphere;
    sphere.mesh_data = sphere_mesh;
    sphere.radius = radius;
    
    float pos_range = CONFIG::BOX_SIZE / 2.0f - radius * 2.0f;
    sphere.position = glm::vec3(
        randomFloat(-pos_range, pos_range),
        randomFloat(-pos_range, pos_range),
        randomFloat(-pos_range, pos_range)
    );

    sphere.velocity = glm::vec3(
        randomFloat(-CONFIG::ICOSPHERE_MAX_START_VELOCITY, CONFIG::ICOSPHERE_MAX_START_VELOCITY),
        randomFloat(-CONFIG::ICOSPHERE_MAX_START_VELOCITY, CONFIG::ICOSPHERE_MAX_START_VELOCITY),
        randomFloat(-CONFIG::ICOSPHERE_MAX_START_VELOCITY, CONFIG::ICOSPHERE_MAX_START_VELOCITY)
    );
    
    return sphere;
}

void updatePhysics(std::vector<SceneObject>& objects, float delta_time) {
    const float BOX_HALF_SIZE = CONFIG::BOX_SIZE / 2.0f;
    const float MAX_VELOCITY = CONFIG::ICOSPHERE_MAX_START_VELOCITY * 3.0f; // in case of runaway
    const float DAMPING = 1.0f; // no elasticity
    const float RESTITUTION = 1.0f; // ^^^
    const float MIN_SEPARATION_VELOCITY = 0.01f;
    
    delta_time = std::min(delta_time, 0.033f); // cap DT to prevent issues if the machine is struggling
    
    for (auto& obj : objects) {
        obj.velocity *= DAMPING;
        
        float speed = glm::length(obj.velocity);
        if (speed > MAX_VELOCITY) {
            obj.velocity = glm::normalize(obj.velocity) * MAX_VELOCITY;
        }
    }

    for (auto& obj : objects) {
        obj.position += obj.velocity * delta_time;
    }

    for (auto& obj : objects) {
        for (int i = 0; i < 3; ++i) {
            if (obj.position[i] - obj.radius < -BOX_HALF_SIZE) {
                obj.position[i] = -BOX_HALF_SIZE + obj.radius;
                obj.velocity[i] *= -RESTITUTION;
            } else if (obj.position[i] + obj.radius > BOX_HALF_SIZE) {
                obj.position[i] = BOX_HALF_SIZE - obj.radius;
                obj.velocity[i] *= -RESTITUTION;
            }
        }
    }

    for (size_t i = 0; i < objects.size(); ++i) {
        for (size_t j = i + 1; j < objects.size(); ++j) {
            auto& obj1 = objects[i];
            auto& obj2 = objects[j];

            glm::vec3 delta = obj2.position - obj1.position;
            float distance = glm::length(delta);
            float combined_radii = obj1.radius + obj2.radius;

            if (distance > 0 && distance < combined_radii) {
                glm::vec3 collision_normal = delta / distance;
                
                float overlap = combined_radii - distance;
                
                float separation_distance = overlap * 0.51f;
                obj1.position -= collision_normal * separation_distance;
                obj2.position += collision_normal * separation_distance;
                
                glm::vec3 relative_velocity = obj2.velocity - obj1.velocity;
                float vel_along_normal = glm::dot(relative_velocity, collision_normal);
                
                if (vel_along_normal > 0) continue;
                
                float impulse_magnitude = -(1.0f + RESTITUTION) * vel_along_normal / 2.0f;
                glm::vec3 impulse = impulse_magnitude * collision_normal;
                
                obj1.velocity -= impulse;
                obj2.velocity += impulse;
                
                float separation_speed = glm::length(obj1.velocity - obj2.velocity);
                if (separation_speed < MIN_SEPARATION_VELOCITY) {
                    obj1.velocity -= collision_normal * MIN_SEPARATION_VELOCITY * 0.5f;
                    obj2.velocity += collision_normal * MIN_SEPARATION_VELOCITY * 0.5f;
                }
            }
        }
    }

    for (auto& obj : objects) {
        float speed = glm::length(obj.velocity);
        if (speed > MAX_VELOCITY) {
            obj.velocity = glm::normalize(obj.velocity) * MAX_VELOCITY;
        }
    }

    for (auto& obj : objects) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, obj.position);
        model = glm::scale(model, glm::vec3(obj.radius)); 
        obj.model_matrix = model;
    }
}


int main() {
    UserState* user = new UserState {
        nullptr, 90.0f, true, true, CONFIG::MIN_FRAMES_PER_CURSOR_TOGGLE,
        -90.0f, 0.0f, 0.0f, 0.0f, {0, 0, 25}, {0, 0, -1}, {0, 1, 0},
        {0, 0, 25}, 0.0f
    };

    GLFWwindow* window = createWindow(user);
    if (!window) return -1;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(messageCallback, 0);

    unsigned int shader_program = createShaderProgram(CONFIG::SHADER_PATH);
    glUseProgram(shader_program);
    setInt(shader_program, "primary_texture", 0);

    setVec3(shader_program, "light_position", glm::vec3(0.0f, 20.0f, 0.0f));
    setVec3(shader_program, "light_color", glm::vec3(1.0f, 1.0f, 1.0f));
    setFloat(shader_program, "constant_attenuation", 1.0f);
    setFloat(shader_program, "linear_attenuation", 0.01f);
    setFloat(shader_program, "quadratic_attenuation", 0.001f);

    setVec3(shader_program, "material.ambient", glm::vec3(0.3f, 0.3f, 0.3f));
    setVec3(shader_program, "material.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
    setVec3(shader_program, "material.specular", glm::vec3(1.0f, 1.0f, 1.0f));
    setFloat(shader_program, "material.shininess", 32.0f);

    setBool(shader_program, "render_wireframe", false);

    unsigned int blueprint_texture = loadTexture(CONFIG::BLUEPRINT_TEXTURE_PATH, false);
    unsigned int white_texture = loadTexture(CONFIG::WHITE_TEXTURE_PATH, false);
    
    Mesh* box_mesh = generateMesh(CONFIG::CUBE_VERTICES);
    
    std::vector<float> icosphere_vertices = readFBXFile(CONFIG::FBX_ICOSPHERE_PATH);
    if (icosphere_vertices.empty()) {
        std::cerr << "Failed to load icosphere model, cannot continue." << std::endl;
        glfwTerminate();
        return -1;
    }
    Mesh* icosphere_mesh = generateMesh(icosphere_vertices);
    
    std::vector<SceneObject> icospheres;
    for(int i = 0; i < CONFIG::NUM_ICOSPHERES; ++i) {
        icospheres.push_back(createIcosphere(icosphere_mesh, CONFIG::ICOSPHERE_RADIUS));
    }

    float last_frame = 0.0f;
    
    while (!glfwWindowShouldClose(window)) {
        float current_frame = (float)glfwGetTime();
        user->delta_time = current_frame - last_frame;
        last_frame = current_frame;
        if (CONFIG::OUT_FPS) {
            std::cout << "FPS: " << 1.0f / user->delta_time << "\r" << std::flush;
        }
        
        processInput(window, user);

        updatePhysics(icospheres, user->delta_time);

        glClearColor(CONFIG::WINDOW_COLOR.r, CONFIG::WINDOW_COLOR.g, CONFIG::WINDOW_COLOR.b, CONFIG::WINDOW_COLOR.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader_program);

        glm::mat4 projection = glm::perspective(glm::radians(user->fov), (float)user->mode->width / (float)user->mode->height, 0.1f, 200.0f);
        glm::mat4 view = glm::lookAt(user->camera_position, user->camera_position + user->camera_front, user->camera_up);
        setMat4(shader_program, "projection", projection);
        setMat4(shader_program, "view", view);
        setVec3(shader_program, "view_position", user->camera_position);
        
        glBindVertexArray(box_mesh->vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blueprint_texture);

        glm::mat4 box_model = glm::mat4(1.0f);
        box_model = glm::scale(box_model, glm::vec3(CONFIG::BOX_SIZE));
        setMat4(shader_program, "model", box_model);

        setBool(shader_program, "render_wireframe", true);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLES, 0, box_mesh->vertex_count);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        setBool(shader_program, "render_wireframe", false);

        glBindVertexArray(icosphere_mesh->vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, white_texture);

        for(const auto& sphere : icospheres) {
            setMat4(shader_program, "model", sphere.model_matrix);
            glDrawArrays(GL_TRIANGLES, 0, icosphere_mesh->vertex_count);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &box_mesh->vao);
    glDeleteBuffers(1, &box_mesh->vbo);
    delete box_mesh;

    glDeleteVertexArrays(1, &icosphere_mesh->vao);
    glDeleteBuffers(1, &icosphere_mesh->vbo);
    delete icosphere_mesh;
    
    delete user;

    glDeleteProgram(shader_program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


void GLAPIENTRY messageCallback([[maybe_unused]] GLenum source, GLenum type, [[maybe_unused]] GLuint id,
                                GLenum severity, [[maybe_unused]] GLsizei length, const GLchar* message, [[maybe_unused]] const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    fprintf(stderr, "gl callback: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "opengl error" : ""),
        type, severity, message);
}




// SHADING

GLenum getShaderType(const std::string& file_extension) {
    if (file_extension == ".vert") return GL_VERTEX_SHADER;
    if (file_extension == ".frag") return GL_FRAGMENT_SHADER;
    std::cerr << "invalid shader extension type: " << file_extension << "\n";
    return 0;
}

std::string stringifyShaderSource(const std::string& file_path) {
    std::ifstream shader_file(file_path);
    if (!shader_file.is_open()) {
        std::cerr << "failed to open shader file at: " + file_path << "\n";
        return "";
    }
    std::stringstream buffer;
    buffer << shader_file.rdbuf();
    return buffer.str();
};

unsigned int compileShader(const std::string& shader_source, GLenum shader_type) {
    unsigned int shader = glCreateShader(shader_type);
    const char *shader_c_str = shader_source.c_str();
    glShaderSource(shader, 1, &shader_c_str, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "shader compilation failed:\n" << infoLog << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

unsigned int createShaderProgram(const std::string& shader_folder_path) {
    std::vector<unsigned int> shaders;
    unsigned int shader_program = glCreateProgram();

    for (const auto& entry : std::filesystem::directory_iterator(shader_folder_path)) {
        if (!entry.is_regular_file()) continue;
        
        std::string file_path = entry.path().string();
        std::string extension = entry.path().extension().string();
        
        GLenum shader_type = getShaderType(extension);
        if(shader_type == 0) continue;

        std::string shader_source = stringifyShaderSource(file_path);
        if (shader_source.empty()) {
            std::cerr << "empty shader source for: " << file_path << "\n";
            continue;
        }
        
        std::cout << "currently compiling shader: " << file_path << std::endl; // Add this line
        unsigned int shader = compileShader(shader_source, shader_type);
        if (!shader) {
            std::cerr << "shader failed to load: " << file_path << "\n";
            continue;
        }
        shaders.push_back(shader);
    }
    
    if (shaders.empty()) {
        std::cerr << "no shaders were successfully compiled!" << std::endl;
        return 0;
    }
    
    for (unsigned int shader : shaders) {
        glAttachShader(shader_program, shader);
        glDeleteShader(shader);
    }

    glLinkProgram(shader_program);

    int success;
    char infoLog[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, nullptr, infoLog);
        std::cerr << "shader program linking failed:\n" << infoLog << "\n";
        return 0;
    }
    
    std::cout << "shader program linked successfully!" << std::endl;
    return shader_program;
}

void setInt(const unsigned int shader_program, const std::string& name, const int value) {
    glUniform1i(glGetUniformLocation(shader_program, name.c_str()), value);
}

void setBool(unsigned int shader_program, const std::string& name, bool value) {
    glUniform1i(glGetUniformLocation(shader_program, name.c_str()), static_cast<int>(value));
}

void setFloat(const unsigned int shader_program, const std::string& name, const float value) {
    glUniform1f(glGetUniformLocation(shader_program, name.c_str()), value);
}

void setMat4(const unsigned int shader_program, const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(glGetUniformLocation(shader_program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void setVec3(const unsigned int shader_program, const std::string& name, const glm::vec3& value) {
    glUniform3fv(glGetUniformLocation(shader_program, name.c_str()), 1, glm::value_ptr(value));
}

// TEXTURING

unsigned int loadTexture(const std::filesystem::path& texture_path, bool upside_down) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(upside_down);
    unsigned char *data = stbi_load(texture_path.string().c_str(), &width, &height, &channels, 0);
    
    if (data) {
        GLenum format = GL_RGB;
        if (channels == 1) format = GL_RED;
        else if (channels == 3) format = GL_RGB;
        else if (channels == 4) format = GL_RGBA;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "failed to load texture at: " << texture_path << " reason: " << stbi_failure_reason() << "\n";
    }
    stbi_image_free(data);
    return texture;
}

// MESHES

std::vector<float> readFBXFile(const std::string& file_path) {
    std::vector<float> mesh_data;
    ufbx_load_opts opts = { };
    ufbx_error error = { };
    ufbx_scene *scene = ufbx_load_file(file_path.c_str(), &opts, &error);

    if (!scene) {
        std::cerr << "failed to load FBX file: " << file_path << " - " << error.description.data << std::endl;
        return mesh_data;
    }

    for (size_t i = 0; i < scene->meshes.count; i++) {
        ufbx_mesh *mesh = scene->meshes.data[i];
        for (ufbx_face face : mesh->faces) {
            std::vector<uint32_t> tri_indices(mesh->max_face_triangles * 3);
            uint32_t num_tris = ufbx_triangulate_face(tri_indices.data(), tri_indices.size(), mesh, face);
            
            for (size_t t = 0; t < num_tris * 3; t++) {
                uint32_t index = tri_indices[t];
                
                ufbx_vec3 position = mesh->vertex_position[index];
                ufbx_vec3 normal = {0, 1, 0};
                if(mesh->vertex_normal.exists) normal = mesh->vertex_normal[index];

                ufbx_vec2 uv = {0, 0};
                if(mesh->vertex_uv.exists) uv = mesh->vertex_uv[index];

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

Mesh* generateMesh(const std::vector<float>& vertices) {
    if (vertices.empty()) {
        std::cerr << "attempted to generate a mesh with no vertex data??" << std::endl;
        return nullptr;
    }
    Mesh* new_mesh = new Mesh();
    new_mesh->vertex_count = vertices.size() / CONFIG::VERTEX_LENGTH;

    glGenVertexArrays(1, &new_mesh->vao);
    glBindVertexArray(new_mesh->vao);

    glGenBuffers(1, &new_mesh->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, new_mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, CONFIG::VERTEX_LENGTH * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, CONFIG::VERTEX_LENGTH * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, CONFIG::VERTEX_LENGTH * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return new_mesh;
}

// WINDOWING AND INPUT

GLFWwindow* createWindow(UserState* user) {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << "\n";
        return nullptr;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    user->mode = glfwGetVideoMode(monitor);
    if (!monitor) {
        std::cerr << "Failed to get primary monitor" << "\n";
        glfwTerminate();
        return nullptr;
    }
    
    glfwWindowHint(GLFW_RED_BITS, user->mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, user->mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, user->mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, user->mode->refreshRate);

    GLFWwindow* window = glfwCreateWindow(user->mode->width, user->mode->height, CONFIG::WINDOW_NAME, monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW Window" << "\n";
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    if (CONFIG::UNCAPPED_FRAMES) glfwSwapInterval(0);
    else glfwSwapInterval(1);

    glfwSetWindowUserPointer(window, user);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialise GLAD" << "\n";
        return nullptr;
    }

    glViewport(0, 0, user->mode->width, user->mode->height);
    user->last_x = user->mode->width / 2.0f;
    user->last_y = user->mode->height / 2.0f;

    return window;
}

void processInput(GLFWwindow* window, UserState* user) {
    user->frames_since_cursor_toggle++;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    } else if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (user->frames_since_cursor_toggle >= CONFIG::MIN_FRAMES_PER_CURSOR_TOGGLE) {
            user->lock_cursor = !user->lock_cursor;
            glfwSetInputMode(window, GLFW_CURSOR, user->lock_cursor ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            user->frames_since_cursor_toggle = 0;
            user->first_mouse = true;
        }
    }

    float camera_speed = CONFIG::CAMERA_MOVEMENT_SPEED * user->delta_time;
    glm::vec3 flat_front = glm::normalize(glm::vec3(user->camera_front.x, 0.0f, user->camera_front.z));
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) user->target_position += flat_front * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) user->target_position -= flat_front * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) user->target_position -= glm::normalize(glm::cross(flat_front, user->camera_up)) * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) user->target_position += glm::normalize(glm::cross(flat_front, user->camera_up)) * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) user->target_position.y += camera_speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) user->target_position.y -= camera_speed; 

    user->camera_position = glm::mix(user->camera_position, user->target_position, CONFIG::CAMERA_LERP_RATE * user->delta_time);
}

void framebufferSizeCallback([[maybe_unused]] GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

glm::vec3 determineFront(float yaw, float pitch) {
    return glm::normalize(glm::vec3{
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    });
}

void mouseCallback(GLFWwindow* window, double x_pos_in, double y_pos_in) {
    UserState* user = static_cast<UserState*>(glfwGetWindowUserPointer(window));
    if (!user->lock_cursor) return;

    float x_pos = static_cast<float>(x_pos_in);
    float y_pos = static_cast<float>(y_pos_in);

    if (user->first_mouse) {
        user->last_x = x_pos;
        user->last_y = y_pos;
        user->first_mouse = false;
    }

    float x_offset = x_pos - user->last_x;
    float y_offset = user->last_y - y_pos;
    user->last_x = x_pos;
    user->last_y = y_pos;

    x_offset *= CONFIG::MOUSE_SENSITIVITY;
    y_offset *= CONFIG::MOUSE_SENSITIVITY;

    user->yaw += x_offset;
    user->pitch = std::clamp(user->pitch + y_offset, -89.0f, 89.0f);
    
    user->camera_front = determineFront(user->yaw, user->pitch);
}

void scrollCallback(GLFWwindow* window, [[maybe_unused]] double x_offset, double y_offset) {
    UserState* user = static_cast<UserState*>(glfwGetWindowUserPointer(window));
    user->fov = std::clamp(user->fov - (float)y_offset * CONFIG::FOV_SCROLL_SPEED, CONFIG::FOV_MIN, CONFIG::FOV_MAX);
}
