cmake_minimum_required(VERSION 3.16)

project(
    OpenGLFireworks
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(
    screensaver
    src/main.cpp
)

target_link_libraries(
    screensaver
    PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        screensaver
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()
