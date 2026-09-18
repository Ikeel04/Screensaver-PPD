#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "config.hpp"

// Estado de una particula individual.
struct Particle {
    float x;      // posicion horizontal (coordenadas del mundo)
    float y;      // posicion vertical
    float vx;     // velocidad horizontal
    float vy;     // velocidad vertical
    float r;      // color base rojo   [0, 1]
    float g;      // color base verde  [0, 1]
    float b;      // color base azul   [0, 1]
    float life;   // vida restante [0, 1]; 0 = apagada
    float decay;  // velocidad a la que pierde vida (por segundo)
    float phase;  // fase para el viento y el parpadeo
    int slot;     // fuego artificial al que pertenece
};

// Formato de cada vertice que se envia a OpenGL.
struct Vertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};

// Un "slot" es un fuego artificial. Cada slot es duenio de un rango
// contiguo de particulas [firstParticle, firstParticle + particleCount).
struct FireworkSlot {
    int firstParticle;
    int particleCount;
    int aliveParticles;      // particulas vivas en el ultimo frame
    float cooldown;          // segundos que faltan para relanzarlo
    std::uint64_t launchId;  // identificador unico de cada explosion
    float centerX;           // centro de la explosion
    float centerY;
    float power;             // velocidad maxima inicial
    float colorR;            // color principal de la explosion
    float colorG;
    float colorB;
};

// Estado completo de la simulacion.
struct SimulationState {
    std::vector<Particle> particles;
    std::vector<Vertex> vertices;     // un vertice por particula (mismo indice)
    std::vector<FireworkSlot> slots;
    std::vector<int> aliveCounts;     // contador por slot (destino de la reduccion)
    std::vector<int> launchList;      // slots que explotan en este frame
    std::mt19937 slotRandom;          // RNG solo para decisiones de slots (hilo maestro)
    std::uint64_t seed = 0;
    float halfWidth = 1.0f;           // mitad del ancho del mundo (relacion de aspecto)
    float time = 0.0f;                // tiempo simulado acumulado
    std::uint64_t launchCounter = 0;  // explosiones generadas en total
};

// Reserva memoria e inicializa todas las particulas apagadas.
void initializeSimulation(SimulationState& state, int particleCount, int fireworkCount,
                          float halfWidth, std::uint64_t seed);

// Parte secuencial comun: revisa que slots terminaron y decide cuales se
// relanzan en este frame (llena launchList). Usa aliveCounts.
void scheduleLaunches(SimulationState& state, float deltaTime);

// Avanza un frame con cada una de las versiones.
void stepSequential(SimulationState& state, float deltaTime);
void stepParallelV1(SimulationState& state, float deltaTime);
void stepParallelV2(SimulationState& state, float deltaTime);

// Elige la version segun el modo.
void stepSimulation(SimulationState& state, SimulationMode mode, float deltaTime);

// Suma de posiciones y vidas; sirve para verificar que las versiones
// paralelas producen el mismo resultado que la secuencial.
double computeChecksum(const SimulationState& state);

// Particulas vivas en total (suma de aliveCounts).
int countAliveParticles(const SimulationState& state);
