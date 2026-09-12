#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Version del algoritmo que se ejecuta en cada frame.
enum class SimulationMode {
    Sequential,  // seq : version secuencial de referencia
    ParallelV1,  // par1: primera version paralela (un parallel for por fase)
    ParallelV2   // par2: version paralela mejorada (una sola region paralela por frame)
};

// Todos los parametros configurables del programa. Se llenan desde la
// linea de comandos para evitar valores hard-coded.
struct Config {
    int particleCount = 5000;          // N: cantidad total de particulas
    int fireworkCount = 6;             // E: fuegos artificiales simultaneos
    int threadCount = 0;               // hilos OpenMP (0 = maximo disponible)
    int windowWidth = 800;             // ancho de la ventana (min 640)
    int windowHeight = 600;            // alto de la ventana (min 480)
    float particleSize = 4.0f;         // tamano de cada particula en pixeles
    SimulationMode mode = SimulationMode::ParallelV2;
    bool vsync = true;                 // sincronizar con el monitor
    bool seedProvided = false;         // true si el usuario dio --seed
    std::uint64_t seed = 0;            // semilla pseudoaleatoria

    // Parametros del modo benchmark.
    bool benchmark = false;            // ejecutar mediciones en vez del screensaver
    bool simulationOnly = false;       // medir sin abrir ventana (sin render)
    int benchmarkRuns = 10;            // mediciones por prueba
    int benchmarkFrames = 300;         // frames medidos por cada medicion
    std::vector<int> benchmarkThreads; // hilos a probar (vacio = automatico)
    std::string csvPath = "results/benchmark.csv";
};

// Resultado del parseo de argumentos.
enum class ParseStatus {
    Ok,
    ShowHelp
};

// Lee argv y llena config. Lanza std::invalid_argument con un mensaje
// descriptivo si algun argumento es invalido.
ParseStatus parseArguments(int argc, char* argv[], Config& config);

// Imprime la ayuda de uso del programa.
void printUsage(const char* programName);

// Convierte el modo a texto corto (seq, par1, par2).
std::string modeToString(SimulationMode mode);
