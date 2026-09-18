#include "simulation.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "physics.hpp"

namespace {

// Convierte un color HSV (h en [0,1)) a RGB. Se usa para generar colores
// vivos y distintos para cada explosion.
void hsvToRgb(float hue, float saturation, float value, float& r, float& g, float& b) {
    const float h = hue * 6.0f;
    const int sector = static_cast<int>(h) % 6;
    const float fraction = h - std::floor(h);
    const float p = value * (1.0f - saturation);
    const float q = value * (1.0f - saturation * fraction);
    const float t = value * (1.0f - saturation * (1.0f - fraction));

    switch (sector) {
        case 0: r = value; g = t; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = t; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = t; g = p; b = value; break;
        default: r = value; g = p; b = q; break;
    }
}

float randomRange(std::mt19937& engine, float minValue, float maxValue) {
    std::uniform_real_distribution<float> distribution(minValue, maxValue);
    return distribution(engine);
}

}  // namespace

void initializeSimulation(SimulationState& state, int particleCount, int fireworkCount,
                          float halfWidth, std::uint64_t seed) {
    state.particles.assign(static_cast<std::size_t>(particleCount), Particle{});
    state.vertices.assign(static_cast<std::size_t>(particleCount), Vertex{});
    state.slots.assign(static_cast<std::size_t>(fireworkCount), FireworkSlot{});
    state.aliveCounts.assign(static_cast<std::size_t>(fireworkCount), 0);
    state.launchList.clear();
    state.launchList.reserve(static_cast<std::size_t>(fireworkCount));
    state.slotRandom.seed(static_cast<std::mt19937::result_type>(seed));
    state.seed = seed;
    state.halfWidth = halfWidth;
    state.time = 0.0f;
    state.launchCounter = 0;

    // Reparte N particulas entre E slots; los primeros reciben el residuo.
    const int basePerSlot = particleCount / fireworkCount;
    const int remainder = particleCount % fireworkCount;
    int nextParticle = 0;

    for (int slotIndex = 0; slotIndex < fireworkCount; ++slotIndex) {
        FireworkSlot& slot = state.slots[static_cast<std::size_t>(slotIndex)];
        slot.firstParticle = nextParticle;
        slot.particleCount = basePerSlot + (slotIndex < remainder ? 1 : 0);
        slot.aliveParticles = 0;
        // Lanzamientos escalonados para que no exploten todos a la vez.
        slot.cooldown = 0.15f + 0.45f * static_cast<float>(slotIndex) +
                        randomRange(state.slotRandom, 0.0f, 0.25f);
        slot.launchId = 0;
        nextParticle += slot.particleCount;
    }

    // Todas las particulas inician apagadas y fuera de pantalla.
    for (std::size_t i = 0; i < state.particles.size(); ++i) {
        state.particles[i].life = 0.0f;
        physics::writeVertex(state.particles[i], state.vertices[i], 0.0f);
    }

    // Los slots de cada particula (necesario para contar vivas por slot).
    for (int slotIndex = 0; slotIndex < fireworkCount; ++slotIndex) {
        const FireworkSlot& slot = state.slots[static_cast<std::size_t>(slotIndex)];
        for (int i = slot.firstParticle; i < slot.firstParticle + slot.particleCount; ++i) {
            state.particles[static_cast<std::size_t>(i)].slot = slotIndex;
        }
    }
}

void scheduleLaunches(SimulationState& state, float deltaTime) {
    // Esta parte es O(E) (E = cantidad de fuegos, maximo 64), por eso se
    // deja secuencial: paralelizarla costaria mas de lo que ahorra.
    state.launchList.clear();

    for (std::size_t slotIndex = 0; slotIndex < state.slots.size(); ++slotIndex) {
        FireworkSlot& slot = state.slots[slotIndex];
        slot.aliveParticles = state.aliveCounts[slotIndex];

        if (slot.aliveParticles > 0) {
            continue;
        }

        slot.cooldown -= deltaTime;
        if (slot.cooldown > 0.0f) {
            continue;
        }

        // Nueva explosion: posicion, potencia y color pseudoaleatorios.
        slot.launchId = ++state.launchCounter;
        slot.centerX = randomRange(state.slotRandom, -0.75f, 0.75f) * state.halfWidth;
        slot.centerY = randomRange(state.slotRandom, -0.05f, 0.65f);
        slot.power = randomRange(state.slotRandom, 0.35f, 0.75f);
        hsvToRgb(randomRange(state.slotRandom, 0.0f, 1.0f),
                 randomRange(state.slotRandom, 0.55f, 0.95f), 1.0f,
                 slot.colorR, slot.colorG, slot.colorB);
        slot.cooldown = randomRange(state.slotRandom, 0.2f, 1.2f);

        // Se considera viva para que no se vuelva a lanzar antes del siguiente conteo.
        slot.aliveParticles = slot.particleCount;
        state.launchList.push_back(static_cast<int>(slotIndex));
    }
}

void stepSimulation(SimulationState& state, SimulationMode mode, float deltaTime) {
    switch (mode) {
        case SimulationMode::Sequential:
            stepSequential(state, deltaTime);
            break;
        case SimulationMode::ParallelV1:
            stepParallelV1(state, deltaTime);
            break;
        case SimulationMode::ParallelV2:
            stepParallelV2(state, deltaTime);
            break;
    }
}

double computeChecksum(const SimulationState& state) {
    double checksum = 0.0;
    for (const Particle& particle : state.particles) {
        checksum += static_cast<double>(particle.x) + static_cast<double>(particle.y) * 2.0 +
                    static_cast<double>(particle.life) * 3.0;
    }
    return checksum;
}

int countAliveParticles(const SimulationState& state) {
    return std::accumulate(state.aliveCounts.begin(), state.aliveCounts.end(), 0);
}
