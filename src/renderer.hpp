#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "simulation.hpp"

// Encapsula la ventana (GLFW) y los objetos de OpenGL. El constructor
// crea todo y el destructor lo libera (RAII), asi no quedan recursos
// abiertos aunque ocurra un error.
// Todas las llamadas a OpenGL se hacen desde el hilo principal, fuera
// de las regiones paralelas, porque el contexto grafico no es thread-safe.
class Renderer {
public:
    Renderer(int width, int height, int maxParticles, bool vsync);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Dibuja los vertices, intercambia buffers y procesa eventos.
    void drawFrame(const std::vector<Vertex>& vertices, float particleSize);

    bool shouldClose() const;
    bool isKeyPressed(int key) const;
    void requestClose();
    void setTitle(const std::string& title);

    // Mitad del ancho del mundo = ancho / alto (evita explosiones ovaladas).
    float worldHalfWidth() const;

private:
    void destroy();

    GLFWwindow* window_ = nullptr;
    GLuint shaderProgram_ = 0;
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLint pointSizeLocation_ = -1;
    GLint halfWidthLocation_ = -1;
    int maxParticles_ = 0;
    float worldHalfWidth_ = 1.0f;
    bool glfwInitialized_ = false;
};
