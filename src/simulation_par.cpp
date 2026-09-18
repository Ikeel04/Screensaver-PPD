// Versiones PARALELAS de la simulacion con OpenMP.
//
// Descomposicion (PCAM):
//  - Particion: una tarea por particula (descomposicion de dominio).
//  - Comunicacion: ninguna entre particulas; solo el conteo de vivas por
//    slot, que se resuelve con una reduccion.
//  - Aglomeracion: se agrupan bloques contiguos de particulas por hilo
//    (schedule static) para aprovechar la cache y evitar false sharing.
//  - Mapeo: un bloque por hilo; todos los hilos hacen el mismo trabajo.

#include <omp.h>

#include <algorithm>
#include <cmath>

#include "physics.hpp"
#include "simulation.hpp"

// ---------------------------------------------------------------------
// PAR1: primera version paralela.
// Se agrega un "#pragma omp parallel for" a cada ciclo de la version
// secuencial. Cada fase abre y cierra su propio equipo de hilos
// (fork-join), por lo que hay 1 + (explosiones del frame) regiones
// paralelas por frame.
// ---------------------------------------------------------------------
void stepParallelV1(SimulationState& state, float deltaTime) {
    const int particleCount = static_cast<int>(state.particles.size());
    const int slotCount = static_cast<int>(state.slots.size());
    const float dragFactor = std::exp(-physics::AIR_DRAG * deltaTime);
    const float time = state.time;
    const float halfWidth = state.halfWidth;

    Particle* particles = state.particles.data();
    Vertex* vertices = state.vertices.data();
    int* aliveCounts = state.aliveCounts.data();

    std::fill(state.aliveCounts.begin(), state.aliveCounts.end(), 0);

    // Fase 1 en paralelo. Cada particula solo escribe su propia posicion
    // en particles[i] y vertices[i], asi que no hay condiciones de carrera.
    // El conteo por slot si es compartido: se usa reduction sobre el
    // arreglo (cada hilo tiene su copia privada y al final se suman).
#pragma omp parallel for schedule(static) reduction(+ : aliveCounts[:slotCount])
    for (int i = 0; i < particleCount; ++i) {
        if (physics::updateParticle(particles[i], deltaTime, time, dragFactor, halfWidth)) {
            ++aliveCounts[particles[i].slot];
        }
        physics::writeVertex(particles[i], vertices[i], time);
    }
    // Barrera implicita: aqui todos los hilos terminaron y la reduccion
    // ya esta combinada en aliveCounts.

    // Fase 2 secuencial (O(E)).
    scheduleLaunches(state, deltaTime);

    // Fase 3: un parallel for por cada explosion nueva.
    for (const int slotIndex : state.launchList) {
        const FireworkSlot& slot = state.slots[static_cast<std::size_t>(slotIndex)];
        const int first = slot.firstParticle;
        const int last = slot.firstParticle + slot.particleCount;
        const std::uint64_t seed = state.seed;

#pragma omp parallel for schedule(static)
        for (int i = first; i < last; ++i) {
            physics::initParticle(particles[i], i, slotIndex, slot, seed);
            physics::writeVertex(particles[i], vertices[i], time);
        }
    }

    state.time += deltaTime;
}

// ---------------------------------------------------------------------
// PAR2: version paralela mejorada.
// Se crea UNA sola region paralela por frame y dentro se reparten las
// fases con directivas de trabajo compartido:
//   omp for + reduction  -> actualizacion y conteo
//   omp single           -> planificacion de lanzamientos (un solo hilo)
//   omp for nowait       -> creacion de particulas de cada explosion
// Asi se paga el costo de crear/sincronizar el equipo de hilos una vez
// por frame en lugar de varias.
// ---------------------------------------------------------------------
void stepParallelV2(SimulationState& state, float deltaTime) {
    const int particleCount = static_cast<int>(state.particles.size());
    const int slotCount = static_cast<int>(state.slots.size());
    const float dragFactor = std::exp(-physics::AIR_DRAG * deltaTime);
    const float time = state.time;
    const float halfWidth = state.halfWidth;
    const std::uint64_t seed = state.seed;

    Particle* particles = state.particles.data();
    Vertex* vertices = state.vertices.data();
    int* aliveCounts = state.aliveCounts.data();

    std::fill(state.aliveCounts.begin(), state.aliveCounts.end(), 0);

#pragma omp parallel
    {
        // Fase 1: actualizacion + conteo con reduccion.
#pragma omp for schedule(static) reduction(+ : aliveCounts[:slotCount])
        for (int i = 0; i < particleCount; ++i) {
            if (physics::updateParticle(particles[i], deltaTime, time, dragFactor, halfWidth)) {
                ++aliveCounts[particles[i].slot];
            }
            physics::writeVertex(particles[i], vertices[i], time);
        }
        // Barrera implicita del omp for: la reduccion termino.

        // Fase 2: un solo hilo decide los lanzamientos. Usa el RNG de slots
        // (estado compartido), por eso no puede ejecutarse en paralelo.
#pragma omp single
        {
            scheduleLaunches(state, deltaTime);
        }
        // Barrera implicita del single: todos ven launchList actualizada.

        // Fase 3: todos los hilos recorren la misma lista de explosiones y
        // se reparten las particulas de cada una. Los rangos de distintos
        // slots no se traslapan, por eso se usa nowait y solo se espera en
        // la barrera final de la region paralela.
        const std::size_t launches = state.launchList.size();
        for (std::size_t k = 0; k < launches; ++k) {
            const int slotIndex = state.launchList[k];
            const FireworkSlot& slot = state.slots[static_cast<std::size_t>(slotIndex)];
            const int first = slot.firstParticle;
            const int last = slot.firstParticle + slot.particleCount;

#pragma omp for schedule(static) nowait
            for (int i = first; i < last; ++i) {
                physics::initParticle(particles[i], i, slotIndex, slot, seed);
                physics::writeVertex(particles[i], vertices[i], time);
            }
        }
    }
    // Barrera implicita al cerrar la region paralela.

    state.time += deltaTime;
}
