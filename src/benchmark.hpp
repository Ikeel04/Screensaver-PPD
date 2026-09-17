#pragma once

#include "config.hpp"
#include "renderer.hpp"

// Ejecuta las mediciones: primero la version secuencial y luego par1 y
// par2 con cada cantidad de hilos. Imprime cada medicion, una tabla con
// speedup y eficiencia, y guarda todo en CSV.
// renderer puede ser nullptr (modo --sim-only, sin ventana).
// Devuelve EXIT_SUCCESS o EXIT_FAILURE.
int runBenchmark(const Config& config, Renderer* renderer);
