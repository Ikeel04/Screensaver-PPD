cmake_minimum_required(VERSION 3.16)

project(
    OpenGLFireworks
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Si no se indica tipo de build, se compila en Release para que las
# mediciones de tiempo sean representativas (-O2/-O3).
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilacion" FORCE)
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenMP REQUIRED)

add_executable(
    screensaver
    src/main.cpp
    src/config.cpp
    src/simulation_common.cpp
    src/simulation_seq.cpp
    src/simulation_par.cpp
    src/renderer.cpp
    src/benchmark.cpp
)

target_include_directories(screensaver PRIVATE src)

target_link_libraries(
    screensaver
    PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
        OpenMP::OpenMP_CXX
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
