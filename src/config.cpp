#include "config.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

// Limites permitidos para cada parametro (programacion defensiva).
constexpr int MIN_PARTICLES = 1;
constexpr int MAX_PARTICLES = 2000000;
constexpr int MIN_FIREWORKS = 1;
constexpr int MAX_FIREWORKS = 64;
constexpr int MIN_THREADS = 1;
constexpr int MAX_THREADS = 256;
constexpr int MIN_WIDTH = 640;
constexpr int MAX_WIDTH = 3840;
constexpr int MIN_HEIGHT = 480;
constexpr int MAX_HEIGHT = 2160;
constexpr int MIN_RUNS = 1;
constexpr int MAX_RUNS = 100;
constexpr int MIN_FRAMES = 10;
constexpr int MAX_FRAMES = 100000;
constexpr float MIN_SIZE = 1.0f;
constexpr float MAX_SIZE = 20.0f;

// Convierte texto a entero validando que todo el texto sea numerico
// y que el valor este dentro de [minValue, maxValue].
int parseInteger(const std::string& text, const std::string& name, int minValue, int maxValue) {
    std::size_t processed = 0;
    long long value = 0;

    try {
        value = std::stoll(text, &processed);
    } catch (const std::exception&) {
        throw std::invalid_argument(name + " debe ser un numero entero (se recibio '" + text + "').");
    }

    if (processed != text.length()) {
        throw std::invalid_argument(name + " contiene caracteres invalidos: '" + text + "'.");
    }

    if (value < minValue || value > maxValue) {
        std::ostringstream message;
        message << name << " debe estar entre " << minValue << " y " << maxValue
                << " (se recibio " << value << ").";
        throw std::invalid_argument(message.str());
    }

    return static_cast<int>(value);
}

// Igual que parseInteger pero para valores con decimales.
float parseFloat(const std::string& text, const std::string& name, float minValue, float maxValue) {
    std::size_t processed = 0;
    float value = 0.0f;

    try {
        value = std::stof(text, &processed);
    } catch (const std::exception&) {
        throw std::invalid_argument(name + " debe ser un numero (se recibio '" + text + "').");
    }

    if (processed != text.length()) {
        throw std::invalid_argument(name + " contiene caracteres invalidos: '" + text + "'.");
    }

    if (value < minValue || value > maxValue) {
        std::ostringstream message;
        message << name << " debe estar entre " << minValue << " y " << maxValue << '.';
        throw std::invalid_argument(message.str());
    }

    return value;
}

// Lee una lista de hilos separada por comas, por ejemplo "2,4,8".
std::vector<int> parseThreadList(const std::string& text) {
    std::vector<int> threads;
    std::stringstream stream(text);
    std::string item;

    while (std::getline(stream, item, ',')) {
        if (item.empty()) {
            throw std::invalid_argument("--thread-list tiene un elemento vacio: '" + text + "'.");
        }
        threads.push_back(parseInteger(item, "--thread-list", MIN_THREADS, MAX_THREADS));
    }

    if (threads.empty()) {
        throw std::invalid_argument("--thread-list no puede estar vacio.");
    }

    // Se ordena y se eliminan repetidos.
    std::sort(threads.begin(), threads.end());
    threads.erase(std::unique(threads.begin(), threads.end()), threads.end());
    return threads;
}

SimulationMode parseMode(const std::string& text) {
    if (text == "seq") {
        return SimulationMode::Sequential;
    }
    if (text == "par1") {
        return SimulationMode::ParallelV1;
    }
    if (text == "par2") {
        return SimulationMode::ParallelV2;
    }
    throw std::invalid_argument("--mode debe ser seq, par1 o par2 (se recibio '" + text + "').");
}

// Devuelve el valor que sigue a una opcion (por ejemplo el 8 en "--threads 8").
std::string requireValue(int& index, int argc, char* argv[]) {
    const std::string option = argv[index];
    if (index + 1 >= argc) {
        throw std::invalid_argument("La opcion " + option + " necesita un valor.");
    }
    ++index;
    return argv[index];
}

}  // namespace

std::string modeToString(SimulationMode mode) {
    switch (mode) {
        case SimulationMode::Sequential:
            return "seq";
        case SimulationMode::ParallelV1:
            return "par1";
        case SimulationMode::ParallelV2:
            return "par2";
    }
    return "desconocido";
}

void printUsage(const char* programName) {
    std::cout
        << "Uso: " << programName << " [N] [opciones]\n\n"
        << "  N                     Cantidad total de particulas (" << MIN_PARTICLES << " - "
        << MAX_PARTICLES << "). Por defecto 5000.\n\n"
        << "Opciones del screensaver:\n"
        << "  --mode seq|par1|par2  Version a ejecutar (por defecto par2)\n"
        << "  --threads T           Hilos OpenMP (" << MIN_THREADS << " - " << MAX_THREADS
        << "). Por defecto el maximo del equipo\n"
        << "  --fireworks E         Fuegos artificiales simultaneos (" << MIN_FIREWORKS << " - "
        << MAX_FIREWORKS << "). Por defecto 6\n"
        << "  --width W             Ancho de ventana (" << MIN_WIDTH << " - " << MAX_WIDTH << ")\n"
        << "  --height H            Alto de ventana (" << MIN_HEIGHT << " - " << MAX_HEIGHT << ")\n"
        << "  --size S              Tamano de particula en pixeles (1 - 20). Por defecto 4\n"
        << "  --seed S              Semilla pseudoaleatoria (entero >= 0)\n"
        << "  --no-vsync            Desactiva la sincronizacion vertical\n\n"
        << "Opciones de benchmark:\n"
        << "  --benchmark           Mide seq, par1 y par2 y calcula speedup y eficiencia\n"
        << "  --runs R              Mediciones por prueba (" << MIN_RUNS << " - " << MAX_RUNS
        << "). Por defecto 10\n"
        << "  --frames F            Frames por medicion (" << MIN_FRAMES << " - " << MAX_FRAMES
        << "). Por defecto 300\n"
        << "  --thread-list L       Hilos a probar, ej. 2,4,8\n"
        << "  --csv ARCHIVO         Archivo CSV de salida (se agregan filas)\n"
        << "  --sim-only            Mide solo la simulacion, sin abrir ventana\n\n"
        << "Controles en ejecucion: 1 = seq, 2 = par1, 3 = par2, ESC = salir\n"
        << "Ejemplos:\n"
        << "  " << programName << " 20000\n"
        << "  " << programName << " 100000 --mode seq\n"
        << "  " << programName << " 200000 --benchmark --thread-list 2,4,8\n";
}

ParseStatus parseArguments(int argc, char* argv[], Config& config) {
    bool particleCountRead = false;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "-h" || argument == "--help") {
            return ParseStatus::ShowHelp;
        } else if (argument == "--mode") {
            config.mode = parseMode(requireValue(index, argc, argv));
        } else if (argument == "--threads") {
            config.threadCount = parseInteger(requireValue(index, argc, argv), "--threads", MIN_THREADS, MAX_THREADS);
        } else if (argument == "--fireworks") {
            config.fireworkCount = parseInteger(requireValue(index, argc, argv), "--fireworks", MIN_FIREWORKS, MAX_FIREWORKS);
        } else if (argument == "--width") {
            config.windowWidth = parseInteger(requireValue(index, argc, argv), "--width", MIN_WIDTH, MAX_WIDTH);
        } else if (argument == "--height") {
            config.windowHeight = parseInteger(requireValue(index, argc, argv), "--height", MIN_HEIGHT, MAX_HEIGHT);
        } else if (argument == "--size") {
            config.particleSize = parseFloat(requireValue(index, argc, argv), "--size", MIN_SIZE, MAX_SIZE);
        } else if (argument == "--seed") {
            config.seed = static_cast<std::uint64_t>(
                parseInteger(requireValue(index, argc, argv), "--seed", 0, 2147483647));
            config.seedProvided = true;
        } else if (argument == "--no-vsync") {
            config.vsync = false;
        } else if (argument == "--benchmark") {
            config.benchmark = true;
        } else if (argument == "--runs") {
            config.benchmarkRuns = parseInteger(requireValue(index, argc, argv), "--runs", MIN_RUNS, MAX_RUNS);
        } else if (argument == "--frames") {
            config.benchmarkFrames = parseInteger(requireValue(index, argc, argv), "--frames", MIN_FRAMES, MAX_FRAMES);
        } else if (argument == "--thread-list") {
            config.benchmarkThreads = parseThreadList(requireValue(index, argc, argv));
        } else if (argument == "--csv") {
            config.csvPath = requireValue(index, argc, argv);
        } else if (argument == "--sim-only") {
            config.simulationOnly = true;
        } else if (argument.rfind("--", 0) == 0 || (argument.size() > 1 && argument[0] == '-' &&
                                                     !std::isdigit(static_cast<unsigned char>(argument[1])))) {
            throw std::invalid_argument("Opcion desconocida: " + argument);
        } else {
            // Argumento posicional: N.
            if (particleCountRead) {
                throw std::invalid_argument("Se recibio mas de un valor para N: " + argument);
            }
            config.particleCount = parseInteger(argument, "N", MIN_PARTICLES, MAX_PARTICLES);
            particleCountRead = true;
        }
    }

    // Validaciones que dependen de mas de un parametro.
    if (config.fireworkCount > config.particleCount) {
        throw std::invalid_argument("La cantidad de fuegos (--fireworks) no puede ser mayor que N.");
    }

    if (config.simulationOnly && !config.benchmark) {
        throw std::invalid_argument("--sim-only solo puede usarse junto con --benchmark.");
    }

    if (!particleCountRead) {
        std::cout << "No se especifico N. Se utilizara el valor por defecto: "
                  << config.particleCount << '\n';
    }

    if (config.benchmark && config.benchmarkRuns < 10) {
        std::cout << "Aviso: la rubrica pide al menos 10 mediciones por prueba (--runs "
                  << config.benchmarkRuns << ").\n";
    }

    return ParseStatus::Ok;
}
