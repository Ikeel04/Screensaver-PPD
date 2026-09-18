// Proyecto #1 - Computacion Paralela y Distribuida (UVG, 2026)
// Screensaver de fuegos artificiales con OpenGL y OpenMP.
//
// Flujo general:
//   1. Leer y validar argumentos (Config).
//   2. Crear la ventana y los recursos de OpenGL (Renderer).
//   3. Ciclo principal: simular (seq/par1/par2) -> dibujar -> mostrar FPS.
//   4. Liberar recursos (destructor de Renderer).
// Con --benchmark, en vez del ciclo interactivo se ejecutan las mediciones.

#include <omp.h>

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

#include "benchmark.hpp"
#include "config.hpp"
#include "renderer.hpp"
#include "simulation.hpp"

namespace {

constexpr float MAX_DELTA_TIME = 0.05f;      // evita saltos si la ventana se pausa
constexpr double TITLE_UPDATE_SECONDS = 0.5;  // cada cuanto se actualiza el titulo
constexpr double CONSOLE_UPDATE_SECONDS = 1.0;

// Cambia de version con las teclas 1, 2 y 3 (solo cuando cambia).
void handleModeKeys(const Renderer& renderer, SimulationMode& mode) {
    SimulationMode requested = mode;

    if (renderer.isKeyPressed(GLFW_KEY_1)) {
        requested = SimulationMode::Sequential;
    } else if (renderer.isKeyPressed(GLFW_KEY_2)) {
        requested = SimulationMode::ParallelV1;
    } else if (renderer.isKeyPressed(GLFW_KEY_3)) {
        requested = SimulationMode::ParallelV2;
    }

    if (requested != mode) {
        mode = requested;
        std::cout << ">> Cambio de version: " << modeToString(mode) << '\n';
    }
}

// Ciclo interactivo del screensaver.
int runScreensaver(const Config& config) {
    Renderer renderer(config.windowWidth, config.windowHeight, config.particleCount, config.vsync);

    SimulationState state;
    initializeSimulation(state, config.particleCount, config.fireworkCount,
                         renderer.worldHalfWidth(), config.seed);

    SimulationMode mode = config.mode;
    const int threads = omp_get_max_threads();

    std::cout << "Screensaver iniciado | N = " << config.particleCount
              << " | fuegos = " << config.fireworkCount << " | version = " << modeToString(mode)
              << " | hilos = " << threads << " | vsync = " << (config.vsync ? "si" : "no")
              << " | semilla = " << config.seed << '\n'
              << "Teclas: 1 = seq, 2 = par1, 3 = par2, ESC = salir\n";

    double previousTime = glfwGetTime();
    double titleTimer = previousTime;
    double consoleTimer = previousTime;
    int framesSinceTitle = 0;
    double simulationSecondsSinceTitle = 0.0;
    double lastFps = 0.0;

    while (!renderer.shouldClose()) {
        if (renderer.isKeyPressed(GLFW_KEY_ESCAPE)) {
            renderer.requestClose();
            break;
        }
        handleModeKeys(renderer, mode);

        const double currentTime = glfwGetTime();
        const float deltaTime =
            std::min(static_cast<float>(currentTime - previousTime), MAX_DELTA_TIME);
        previousTime = currentTime;

        // Simulacion (la parte que se paraleliza) medida por separado del render.
        const double stepStart = omp_get_wtime();
        stepSimulation(state, mode, deltaTime);
        simulationSecondsSinceTitle += omp_get_wtime() - stepStart;

        // Render: siempre en el hilo principal.
        renderer.drawFrame(state.vertices, config.particleSize);
        ++framesSinceTitle;

        // Despliegue de FPS en el titulo y en consola.
        const double elapsed = currentTime - titleTimer;
        if (elapsed >= TITLE_UPDATE_SECONDS) {
            lastFps = framesSinceTitle / elapsed;
            const double simulationMs = simulationSecondsSinceTitle * 1000.0 / framesSinceTitle;

            std::ostringstream title;
            title << "OpenGL Fireworks | " << modeToString(mode) << " ("
                  << (mode == SimulationMode::Sequential ? 1 : threads) << " hilos)"
                  << " | N = " << config.particleCount << " | FPS = " << std::fixed
                  << std::setprecision(1) << lastFps << " | sim = " << std::setprecision(2)
                  << simulationMs << " ms";
            renderer.setTitle(title.str());

            if (currentTime - consoleTimer >= CONSOLE_UPDATE_SECONDS) {
                std::cout << "FPS = " << std::fixed << std::setprecision(2) << lastFps
                          << " | sim = " << simulationMs << " ms | " << modeToString(mode) << '\n';
                consoleTimer = currentTime;
            }

            framesSinceTitle = 0;
            simulationSecondsSinceTitle = 0.0;
            titleTimer = currentTime;
        }
    }

    std::cout << "Screensaver finalizado.\n";
    return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char* argv[]) {
    Config config;

    // 1. Captura y validacion de argumentos.
    try {
        if (parseArguments(argc, argv, config) == ParseStatus::ShowHelp) {
            printUsage(argv[0]);
            return EXIT_SUCCESS;
        }
    } catch (const std::invalid_argument& error) {
        std::cerr << "Error: " << error.what() << "\n"
                  << "Use " << argv[0] << " --help para ver las opciones.\n";
        return EXIT_FAILURE;
    }

    // Semilla aleatoria si el usuario no dio una.
    if (!config.seedProvided) {
        config.seed = static_cast<std::uint64_t>(std::random_device{}());
    }

    // Cantidad de hilos para las versiones paralelas.
    if (config.threadCount > 0) {
        omp_set_num_threads(config.threadCount);
    }

    try {
        if (config.benchmark) {
            if (config.simulationOnly) {
                return runBenchmark(config, nullptr);
            }
            // En benchmark se desactiva vsync para medir los FPS reales.
            Renderer renderer(config.windowWidth, config.windowHeight, config.particleCount, false);
            return runBenchmark(config, &renderer);
        }

        return runScreensaver(config);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
