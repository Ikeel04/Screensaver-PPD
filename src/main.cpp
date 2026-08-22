#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr int DEFAULT_PARTICLES = 500;
constexpr int MAX_PARTICLES = 200000;
constexpr float GRAVITY = -0.65f;
constexpr float PARTICLE_SIZE = 7.0f;
constexpr float PI = 3.14159265358979323846f;

struct Particle {
    float x;
    float y;
    float vx;
    float vy;
    float r;
    float g;
    float b;
    float life;
};

struct Vertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};

std::mt19937 randomEngine(std::random_device{}());

float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(randomEngine);
}

int parseParticleCount(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "No se especifico N. Se utilizara el valor por defecto: "
                  << DEFAULT_PARTICLES << '\n';
        return DEFAULT_PARTICLES;
    }

    try {
        const std::string argument = argv[1];
        std::size_t processedCharacters = 0;
        const int particleCount = std::stoi(argument, &processedCharacters);

        if (processedCharacters != argument.length()) {
            throw std::invalid_argument("El argumento contiene caracteres invalidos.");
        }

        if (particleCount <= 0 || particleCount > MAX_PARTICLES) {
            throw std::out_of_range("N fuera del rango permitido.");
        }

        return particleCount;
    } catch (const std::exception&) {
        std::cerr << "Error: N debe ser un entero entre 1 y "
                  << MAX_PARTICLES << ".\n";
        std::exit(EXIT_FAILURE);
    }
}

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Error compilando shader:\n" + log);
    }

    return shader;
}

GLuint createShaderProgram() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec4 color;

        out vec4 particleColor;
        uniform float pointSize;

        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
            gl_PointSize = pointSize;
            particleColor = color;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec4 particleColor;
        out vec4 fragmentColor;

        void main() {
            vec2 centerOffset = gl_PointCoord - vec2(0.5);
            if (length(centerOffset) > 0.5) {
                discard;
            }

            float distanceFromCenter = length(centerOffset) * 2.0;
            float brightness = 1.0 - distanceFromCenter * 0.35;
            fragmentColor = vec4(
                particleColor.rgb * brightness,
                particleColor.a
            );
        }
    )";

    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    const GLuint shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success = GL_FALSE;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetProgramInfoLog(shaderProgram, logLength, nullptr, log.data());
        glDeleteProgram(shaderProgram);
        throw std::runtime_error("Error enlazando shader program:\n" + log);
    }

    return shaderProgram;
}

void createExplosion(std::vector<Particle>& particles, float centerX, float centerY) {
    for (Particle& particle : particles) {
        const float angle = randomFloat(0.0f, 2.0f * PI);
        const float speed = randomFloat(0.20f, 0.85f);

        particle.x = centerX;
        particle.y = centerY;
        particle.vx = std::cos(angle) * speed;
        particle.vy = std::sin(angle) * speed;

        // Evita colores demasiado oscuros sobre el fondo.
        particle.r = randomFloat(0.35f, 1.0f);
        particle.g = randomFloat(0.35f, 1.0f);
        particle.b = randomFloat(0.35f, 1.0f);
        particle.life = randomFloat(0.7f, 1.0f);
    }
}

void updateParticles(std::vector<Particle>& particles, float deltaTime) {
    // Esta seccion se mantiene secuencial en Entrega 2.
    // Es el candidato natural para #pragma omp parallel for en una entrega posterior.
    for (Particle& particle : particles) {
        if (particle.life <= 0.0f) {
            continue;
        }

        // Cinematica basica: v = v0 + g*dt; p = p0 + v*dt.
        particle.vy += GRAVITY * deltaTime;
        particle.x += particle.vx * deltaTime;
        particle.y += particle.vy * deltaTime;

        // Desvanecimiento progresivo.
        particle.life -= 0.40f * deltaTime;
        particle.life = std::max(0.0f, particle.life);
    }
}

bool explosionFinished(const std::vector<Particle>& particles) {
    return std::none_of(
        particles.begin(),
        particles.end(),
        [](const Particle& particle) { return particle.life > 0.0f; }
    );
}

void fillVertexBuffer(const std::vector<Particle>& particles, std::vector<Vertex>& vertices) {
    vertices.clear();
    vertices.reserve(particles.size());

    for (const Particle& particle : particles) {
        if (particle.life <= 0.0f) {
            continue;
        }

        vertices.push_back({
            particle.x,
            particle.y,
            particle.r,
            particle.g,
            particle.b,
            particle.life
        });
    }
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(int argc, char* argv[]) {
    const int particleCount = parseParticleCount(argc, argv);

    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Error inicializando GLFW.\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "OpenGL Fireworks",
        nullptr,
        nullptr
    );

    if (window == nullptr) {
        std::cerr << "Error creando la ventana.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Sincroniza el renderizado con el refresco del monitor durante la POC.
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Error inicializando GLEW.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint shaderProgram = 0;
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;

    try {
        shaderProgram = createShaderProgram();

        glGenVertexArrays(1, &vertexArray);
        glGenBuffers(1, &vertexBuffer);

        glBindVertexArray(vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(particleCount * sizeof(Vertex)),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(0)
        );
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(2 * sizeof(float))
        );
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        std::vector<Particle> particles(static_cast<std::size_t>(particleCount));
        std::vector<Vertex> vertices;

        createExplosion(
            particles,
            randomFloat(-0.55f, 0.55f),
            randomFloat(-0.10f, 0.55f)
        );

        double previousFrameTime = glfwGetTime();
        double fpsTimer = previousFrameTime;
        int renderedFrames = 0;

        while (!glfwWindowShouldClose(window)) {
            processInput(window);

            const double currentTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentTime - previousFrameTime);
            previousFrameTime = currentTime;

            // Evita saltos grandes si la aplicacion se pausa o pierde foco.
            deltaTime = std::min(deltaTime, 0.05f);

            updateParticles(particles, deltaTime);

            if (explosionFinished(particles)) {
                createExplosion(
                    particles,
                    randomFloat(-0.55f, 0.55f),
                    randomFloat(-0.10f, 0.55f)
                );
            }

            fillVertexBuffer(particles, vertices);

            glClearColor(0.015f, 0.018f, 0.075f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(shaderProgram);
            const GLint pointSizeLocation = glGetUniformLocation(shaderProgram, "pointSize");
            glUniform1f(pointSizeLocation, PARTICLE_SIZE);

            glBindVertexArray(vertexArray);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                vertices.data()
            );

            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertices.size()));

            glfwSwapBuffers(window);
            glfwPollEvents();

            ++renderedFrames;
            const double fpsElapsed = currentTime - fpsTimer;

            if (fpsElapsed >= 0.5) {
                const double fps = renderedFrames / fpsElapsed;
                std::ostringstream title;
                title << "OpenGL Fireworks"
                      << " | N = " << particleCount
                      << " | FPS = " << std::fixed << std::setprecision(1) << fps;

                glfwSetWindowTitle(window, title.str().c_str());
                renderedFrames = 0;
                fpsTimer = currentTime;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';

        if (vertexBuffer != 0) {
            glDeleteBuffers(1, &vertexBuffer);
        }
        if (vertexArray != 0) {
            glDeleteVertexArrays(1, &vertexArray);
        }
        if (shaderProgram != 0) {
            glDeleteProgram(shaderProgram);
        }

        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
