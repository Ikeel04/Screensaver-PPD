#include "renderer.hpp"

#include <algorithm>
#include <stdexcept>

namespace {

// Vertex shader: convierte coordenadas del mundo a coordenadas de
// OpenGL dividiendo x entre la mitad del ancho (relacion de aspecto).
const char* VERTEX_SHADER_SOURCE = R"(
    #version 330 core
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec4 color;

    out vec4 particleColor;
    uniform float pointSize;
    uniform float worldHalfWidth;

    void main() {
        gl_Position = vec4(position.x / worldHalfWidth, position.y, 0.0, 1.0);
        gl_PointSize = pointSize;
        particleColor = color;
    }
)";

// Fragment shader: dibuja cada punto como un circulo con borde suave.
const char* FRAGMENT_SHADER_SOURCE = R"(
    #version 330 core
    in vec4 particleColor;
    out vec4 fragmentColor;

    void main() {
        vec2 centerOffset = gl_PointCoord - vec2(0.5);
        float distanceFromCenter = length(centerOffset) * 2.0;
        if (distanceFromCenter > 1.0) {
            discard;
        }

        float glow = 1.0 - distanceFromCenter * distanceFromCenter;
        fragmentColor = vec4(particleColor.rgb, particleColor.a * glow);
    }
)";

// Compila un shader y lanza una excepcion con el log si falla.
GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(logLength > 0 ? logLength : 1), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Error compilando shader:\n" + log);
    }

    return shader;
}

// Crea el programa de shaders (vertex + fragment).
GLuint createShaderProgram() {
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE);
    GLuint fragmentShader = 0;

    try {
        fragmentShader = compileShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE);
    } catch (...) {
        glDeleteShader(vertexShader);
        throw;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Una vez enlazados ya no se necesitan los shaders individuales.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(logLength > 0 ? logLength : 1), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        glDeleteProgram(program);
        throw std::runtime_error("Error enlazando shader program:\n" + log);
    }

    return program;
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

}  // namespace

Renderer::Renderer(int width, int height, int maxParticles, bool vsync)
    : maxParticles_(maxParticles),
      worldHalfWidth_(static_cast<float>(width) / static_cast<float>(height)) {
    try {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("Error inicializando GLFW.");
        }
        glfwInitialized_ = true;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        window_ = glfwCreateWindow(width, height, "OpenGL Fireworks", nullptr, nullptr);
        if (window_ == nullptr) {
            throw std::runtime_error("Error creando la ventana (se requiere OpenGL 3.3).");
        }

        glfwMakeContextCurrent(window_);
        glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);

        // vsync = 1 limita a la frecuencia del monitor; 0 deja correr libre
        // (necesario para medir cuantos FPS se alcanzan realmente).
        glfwSwapInterval(vsync ? 1 : 0);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("Error inicializando GLEW.");
        }
        // glewInit puede dejar un error de OpenGL pendiente en core profile.
        glGetError();

        int framebufferWidth = width;
        int framebufferHeight = height;
        glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
        glViewport(0, 0, framebufferWidth, framebufferHeight);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        // Mezcla aditiva: las zonas con muchas particulas brillan mas.
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        shaderProgram_ = createShaderProgram();
        pointSizeLocation_ = glGetUniformLocation(shaderProgram_, "pointSize");
        halfWidthLocation_ = glGetUniformLocation(shaderProgram_, "worldHalfWidth");

        glGenVertexArrays(1, &vertexArray_);
        glGenBuffers(1, &vertexBuffer_);

        glBindVertexArray(vertexArray_);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(static_cast<std::size_t>(maxParticles_) * sizeof(Vertex)),
                     nullptr, GL_DYNAMIC_DRAW);

        // Atributo 0: posicion (x, y).
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);

        // Atributo 1: color (r, g, b, a).
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    } catch (...) {
        // El destructor no se ejecuta si el constructor falla, por eso se
        // libera manualmente lo que se haya creado hasta este punto.
        destroy();
        throw;
    }
}

Renderer::~Renderer() {
    destroy();
}

void Renderer::destroy() {
    if (window_ != nullptr) {
        if (vertexBuffer_ != 0) {
            glDeleteBuffers(1, &vertexBuffer_);
            vertexBuffer_ = 0;
        }
        if (vertexArray_ != 0) {
            glDeleteVertexArrays(1, &vertexArray_);
            vertexArray_ = 0;
        }
        if (shaderProgram_ != 0) {
            glDeleteProgram(shaderProgram_);
            shaderProgram_ = 0;
        }
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    if (glfwInitialized_) {
        glfwTerminate();
        glfwInitialized_ = false;
    }
}

void Renderer::drawFrame(const std::vector<Vertex>& vertices, float particleSize) {
    const std::size_t vertexCount =
        std::min(vertices.size(), static_cast<std::size_t>(maxParticles_));

    glClearColor(0.010f, 0.012f, 0.050f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram_);
    glUniform1f(pointSizeLocation_, particleSize);
    glUniform1f(halfWidthLocation_, worldHalfWidth_);

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);

    // Se sube el arreglo completo de vertices calculado por la simulacion.
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(vertexCount * sizeof(Vertex)), vertices.data());
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertexCount));
    glBindVertexArray(0);

    glfwSwapBuffers(window_);
    glfwPollEvents();
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window_) == GLFW_TRUE;
}

bool Renderer::isKeyPressed(int key) const {
    return glfwGetKey(window_, key) == GLFW_PRESS;
}

void Renderer::requestClose() {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void Renderer::setTitle(const std::string& title) {
    glfwSetWindowTitle(window_, title.c_str());
}

float Renderer::worldHalfWidth() const {
    return worldHalfWidth_;
}
