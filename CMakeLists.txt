cmake_minimum_required(VERSION 3.14)

project(Renderer VERSION 1.0 LANGUAGES CXX C) 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/glad
)

set(GLAD_SOURCE ${CMAKE_SOURCE_DIR}/src/glad.c)

set(UFBX_SOURCE ${CMAKE_SOURCE_DIR}/src/ufbx.c)


file(GLOB_RECURSE PROJECT_SOURCES 
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME} 
    ${PROJECT_SOURCES}
    ${GLAD_SOURCE}
    ${UFBX_SOURCE}
)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    ${CMAKE_DL_LIBS}
    pthread
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
