#include "benchmark.hpp"

#include <omp.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "simulation.hpp"

namespace {

constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;  // dt fijo: todas las versiones simulan lo mismo
constexpr int WARMUP_FRAMES = 30;                  // frames que no se miden (calentar cache)
constexpr double CHECKSUM_TOLERANCE = 1e-9;        // diferencia relativa aceptada
constexpr std::uint64_t DEFAULT_BENCHMARK_SEED = 2026;

// Resultado de una sola medicion.
struct RunResult {
    SimulationMode mode;
    int threads;
    int run;
    double simulationSeconds;  // tiempo total dentro de stepSimulation
    double totalSeconds;       // tiempo total de los frames (simulacion + render)
    double checksum;
};

// Promedios de una prueba (mismo modo y cantidad de hilos).
struct TestSummary {
    SimulationMode mode;
    int threads;
    double meanSimulationMs;
    double stdSimulationMs;
    double maxSimulationMs;
    double meanFrameMs;
    double meanFps;
    bool checksumOk;
};

// Se lanza si el usuario cierra la ventana a media medicion.
struct BenchmarkCancelled {};

// Lista por defecto de hilos: potencias de 2 hasta el total de nucleos,
// mas el total de nucleos si no es potencia de 2.
std::vector<int> defaultThreadList() {
    const int processors = std::max(1, omp_get_num_procs());
    std::vector<int> threads;

    for (int count = 2; count <= processors; count *= 2) {
        threads.push_back(count);
    }
    if (threads.empty() || threads.back() != processors) {
        threads.push_back(processors);
    }

    threads.erase(std::unique(threads.begin(), threads.end()), threads.end());
    return threads;
}

// Ejecuta una medicion completa y devuelve sus tiempos.
RunResult measureRun(const Config& config, Renderer* renderer, SimulationMode mode,
                     int threads, int run, std::uint64_t seed, float halfWidth) {
    omp_set_num_threads(threads);

    SimulationState state;
    initializeSimulation(state, config.particleCount, config.fireworkCount, halfWidth, seed);

    // Calentamiento: mismo trabajo para todas las versiones, no se mide.
    for (int frame = 0; frame < WARMUP_FRAMES; ++frame) {
        stepSimulation(state, mode, FIXED_DELTA_TIME);
    }

    double simulationSeconds = 0.0;
    const double startTime = omp_get_wtime();

    for (int frame = 0; frame < config.benchmarkFrames; ++frame) {
        const double stepStart = omp_get_wtime();
        stepSimulation(state, mode, FIXED_DELTA_TIME);
        simulationSeconds += omp_get_wtime() - stepStart;

        if (renderer != nullptr) {
            renderer->drawFrame(state.vertices, config.particleSize);
            if (renderer->shouldClose() || renderer->isKeyPressed(GLFW_KEY_ESCAPE)) {
                throw BenchmarkCancelled{};
            }
        }
    }

    const double totalSeconds = omp_get_wtime() - startTime;
    return {mode, threads, run, simulationSeconds, totalSeconds, computeChecksum(state)};
}

TestSummary summarize(const std::vector<RunResult>& runs, int frames, double referenceChecksum) {
    TestSummary summary{runs.front().mode, runs.front().threads, 0.0, 0.0, 0.0, 0.0, 0.0, true};
    const double count = static_cast<double>(runs.size());

    for (const RunResult& run : runs) {
        const double simulationMs = run.simulationSeconds * 1000.0 / frames;
        summary.meanSimulationMs += simulationMs / count;
        summary.maxSimulationMs = std::max(summary.maxSimulationMs, simulationMs);
        summary.meanFrameMs += run.totalSeconds * 1000.0 / frames / count;
        summary.meanFps += frames / run.totalSeconds / count;

        const double difference = std::fabs(run.checksum - referenceChecksum);
        const double scale = std::max(1.0, std::fabs(referenceChecksum));
        if (difference / scale > CHECKSUM_TOLERANCE) {
            summary.checksumOk = false;
        }
    }

    for (const RunResult& run : runs) {
        const double simulationMs = run.simulationSeconds * 1000.0 / frames;
        const double deviation = simulationMs - summary.meanSimulationMs;
        summary.stdSimulationMs += deviation * deviation / count;
    }
    summary.stdSimulationMs = std::sqrt(summary.stdSimulationMs);
    return summary;
}

// Abre un CSV en modo "agregar"; escribe el encabezado si el archivo es nuevo.
std::ofstream openCsv(const std::string& path, const std::string& header) {
    const std::filesystem::path filePath(path);
    if (filePath.has_parent_path()) {
        std::filesystem::create_directories(filePath.parent_path());
    }

    const bool isNewFile = !std::filesystem::exists(filePath) || std::filesystem::file_size(filePath) == 0;
    std::ofstream file(path, std::ios::app);
    if (!file) {
        throw std::runtime_error("No se pudo abrir el archivo CSV: " + path);
    }
    if (isNewFile) {
        file << header << '\n';
    }
    return file;
}

std::string summaryPathFor(const std::string& csvPath) {
    const std::filesystem::path path(csvPath);
    std::filesystem::path summaryPath = path.parent_path() / (path.stem().string() + "_resumen.csv");
    return summaryPath.string();
}

}  // namespace

int runBenchmark(const Config& config, Renderer* renderer) {
    const std::vector<int> threadList =
        config.benchmarkThreads.empty() ? defaultThreadList() : config.benchmarkThreads;
    const std::uint64_t seed = config.seedProvided ? config.seed : DEFAULT_BENCHMARK_SEED;
    const float halfWidth = static_cast<float>(config.windowWidth) / static_cast<float>(config.windowHeight);
    const bool rendering = renderer != nullptr;

    std::cout << "\n=== BENCHMARK ===\n"
              << "N = " << config.particleCount << " | fuegos = " << config.fireworkCount
              << " | mediciones = " << config.benchmarkRuns << " | frames por medicion = "
              << config.benchmarkFrames << " | render = " << (rendering ? "si" : "no")
              << " | nucleos disponibles = " << omp_get_num_procs() << "\n"
              << "Hilos a probar:";
    for (const int threads : threadList) {
        std::cout << ' ' << threads;
    }
    std::cout << "\n\n";

    // Pruebas a ejecutar: seq con 1 hilo y luego par1/par2 con cada cantidad.
    std::vector<std::pair<SimulationMode, int>> tests;
    tests.emplace_back(SimulationMode::Sequential, 1);
    for (const int threads : threadList) {
        tests.emplace_back(SimulationMode::ParallelV1, threads);
        tests.emplace_back(SimulationMode::ParallelV2, threads);
    }

    std::vector<TestSummary> summaries;
    double referenceChecksum = 0.0;

    try {
        std::ofstream csv = openCsv(
            config.csvPath,
            "N,fuegos,frames,modo,hilos,medicion,render,sim_total_s,sim_ms_frame,frame_ms,fps,checksum");

        for (const auto& [mode, threads] : tests) {
            std::vector<RunResult> testRuns;

            for (int run = 1; run <= config.benchmarkRuns; ++run) {
                const RunResult result = measureRun(config, renderer, mode, threads, run, seed, halfWidth);
                testRuns.push_back(result);

                if (mode == SimulationMode::Sequential && run == 1) {
                    referenceChecksum = result.checksum;
                }

                const double simulationMs = result.simulationSeconds * 1000.0 / config.benchmarkFrames;
                const double frameMs = result.totalSeconds * 1000.0 / config.benchmarkFrames;
                const double fps = config.benchmarkFrames / result.totalSeconds;

                std::cout << std::fixed << std::setprecision(3)
                          << "[" << std::setw(4) << modeToString(mode) << " | hilos " << std::setw(3) << threads
                          << "] medicion " << std::setw(2) << run << "/" << config.benchmarkRuns
                          << " | sim = " << std::setw(8) << simulationMs << " ms/frame"
                          << " | frame = " << std::setw(8) << frameMs << " ms"
                          << " | FPS = " << std::setprecision(1) << std::setw(8) << fps << '\n';

                csv << config.particleCount << ',' << config.fireworkCount << ',' << config.benchmarkFrames << ','
                    << modeToString(mode) << ',' << threads << ',' << run << ',' << (rendering ? 1 : 0) << ','
                    << std::setprecision(6) << result.simulationSeconds << ',' << simulationMs << ','
                    << frameMs << ',' << fps << ',' << std::setprecision(10) << result.checksum << '\n';
            }

            summaries.push_back(summarize(testRuns, config.benchmarkFrames, referenceChecksum));
            std::cout << '\n';
        }
    } catch (const BenchmarkCancelled&) {
        std::cout << "\nBenchmark cancelado por el usuario. No se genera resumen.\n";
        return EXIT_SUCCESS;
    }

    // Tabla final. Speedup = T_secuencial / T_paralelo; eficiencia = speedup / hilos.
    const TestSummary& sequential = summaries.front();

    std::cout << "=== RESUMEN (N = " << config.particleCount << ") ===\n"
              << "Speedup calculado con el PROMEDIO de las mediciones del tiempo de simulacion.\n\n"
              << std::left << std::setw(6) << "modo" << std::setw(7) << "hilos" << std::right
              << std::setw(12) << "sim(ms)" << std::setw(10) << "desv" << std::setw(10) << "speedup"
              << std::setw(12) << "eficiencia" << std::setw(11) << "frame(ms)" << std::setw(10) << "FPS"
              << std::setw(12) << "speedupFPS" << std::setw(11) << "resultado" << '\n';

    std::ofstream summaryCsv = openCsv(
        summaryPathFor(config.csvPath),
        "N,fuegos,frames,render,modo,hilos,sim_ms_promedio,sim_ms_desv,sim_ms_max,speedup,eficiencia,"
        "speedup_max,frame_ms_promedio,fps_promedio,speedup_fps,checksum_ok");

    for (const TestSummary& summary : summaries) {
        const double speedup = sequential.meanSimulationMs / summary.meanSimulationMs;
        const double efficiency = speedup / summary.threads;
        const double speedupMax = sequential.maxSimulationMs / summary.maxSimulationMs;
        const double speedupFps = summary.meanFps / sequential.meanFps;

        std::cout << std::left << std::setw(6) << modeToString(summary.mode) << std::setw(7) << summary.threads
                  << std::right << std::fixed << std::setprecision(3) << std::setw(12) << summary.meanSimulationMs
                  << std::setw(10) << summary.stdSimulationMs << std::setw(10) << speedup << std::setw(12)
                  << efficiency << std::setw(11) << summary.meanFrameMs << std::setprecision(1) << std::setw(10)
                  << summary.meanFps << std::setprecision(3) << std::setw(12) << speedupFps << std::setw(11)
                  << (summary.checksumOk ? "igual" : "DISTINTO") << '\n';

        summaryCsv << config.particleCount << ',' << config.fireworkCount << ',' << config.benchmarkFrames << ','
                   << (rendering ? 1 : 0) << ',' << modeToString(summary.mode) << ',' << summary.threads << ','
                   << std::setprecision(6) << summary.meanSimulationMs << ',' << summary.stdSimulationMs << ','
                   << summary.maxSimulationMs << ',' << speedup << ',' << efficiency << ',' << speedupMax << ','
                   << summary.meanFrameMs << ',' << summary.meanFps << ',' << speedupFps << ','
                   << (summary.checksumOk ? 1 : 0) << '\n';
    }

    std::cout << "\n'resultado' compara el estado final contra la version secuencial "
                 "(igual = las versiones paralelas calculan lo mismo).\n"
              << "Mediciones guardadas en: " << config.csvPath << '\n'
              << "Resumen guardado en:     " << summaryPathFor(config.csvPath) << "\n";

    const bool allCorrect = std::all_of(summaries.begin(), summaries.end(),
                                        [](const TestSummary& summary) { return summary.checksumOk; });
    return allCorrect ? EXIT_SUCCESS : EXIT_FAILURE;
}
