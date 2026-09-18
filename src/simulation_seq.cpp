// Version SECUENCIAL de la simulacion (linea base para el speedup).
// No usa ninguna directiva de OpenMP.

#include <algorithm>
#include <cmath>

#include "physics.hpp"
#include "simulation.hpp"

void stepSequential(SimulationState& state, float deltaTime) {
    const int particleCount = static_cast<int>(state.particles.size());
    const float dragFactor = std::exp(-physics::AIR_DRAG * deltaTime);
    const float time = state.time;
    const float halfWidth = state.halfWidth;

    Particle* particles = state.particles.data();
    Vertex* vertices = state.vertices.data();
    int* aliveCounts = state.aliveCounts.data();

    std::fill(state.aliveCounts.begin(), state.aliveCounts.end(), 0);

    // Fase 1: actualizar cada particula, contar las vivas por slot y
    // escribir su vertice.
    for (int i = 0; i < particleCount; ++i) {
        if (physics::updateParticle(particles[i], deltaTime, time, dragFactor, halfWidth)) {
            ++aliveCounts[particles[i].slot];
        }
        physics::writeVertex(particles[i], vertices[i], time);
    }

    // Fase 2: decidir que fuegos se relanzan.
    scheduleLaunches(state, deltaTime);

    // Fase 3: generar las particulas de las nuevas explosiones.
    for (const int slotIndex : state.launchList) {
        const FireworkSlot& slot = state.slots[static_cast<std::size_t>(slotIndex)];
        const int first = slot.firstParticle;
        const int last = slot.firstParticle + slot.particleCount;

        for (int i = first; i < last; ++i) {
            physics::initParticle(particles[i], i, slotIndex, slot, state.seed);
            physics::writeVertex(particles[i], vertices[i], time);
        }
    }

    state.time += deltaTime;
}
