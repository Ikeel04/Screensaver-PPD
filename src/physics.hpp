#pragma once

// Funciones que trabajan sobre UNA sola particula. Las usan las tres
// versiones (seq, par1, par2) para que todas hagan exactamente el mismo
// calculo y la unica diferencia sea como se reparte el trabajo.
// Ninguna funcion toca memoria compartida fuera de su propia particula,
// por eso se pueden llamar desde varios hilos sin sincronizacion.

#include <cmath>
#include <cstdint>

#include "simulation.hpp"

namespace physics {

constexpr float PI = 3.14159265358979323846f;
constexpr float GRAVITY = -0.55f;            // aceleracion vertical (unidades/s^2)
constexpr float AIR_DRAG = 0.85f;            // coeficiente de resistencia del aire
constexpr float WIND_STRENGTH = 0.22f;       // amplitud del viento
constexpr float WIND_FREQUENCY = 1.3f;       // frecuencia temporal del viento
constexpr float WIND_WAVE = 2.5f;            // variacion del viento con la altura
constexpr float TWINKLE_FREQUENCY = 14.0f;   // frecuencia del parpadeo
constexpr float RESTITUTION = 0.45f;         // energia que conserva al rebotar
constexpr float FLOOR_FRICTION = 0.70f;      // friccion horizontal al tocar el suelo
constexpr float WORLD_BOTTOM = -1.0f;
constexpr float HIDDEN_POSITION = -10.0f;    // fuera de pantalla (OpenGL lo recorta)

// ---------------------------------------------------------------------
// Generador pseudoaleatorio sin estado compartido (SplitMix64).
// Cada particula calcula sus numeros a partir de (semilla, explosion,
// indice), asi que no hay condiciones de carrera y el resultado es igual
// sin importar cuantos hilos se usen.
// ---------------------------------------------------------------------
inline std::uint64_t splitMix64(std::uint64_t value) {
    value += 0x9E3779B97F4A7C15ull;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;
    return value ^ (value >> 31);
}

struct ParticleRandom {
    std::uint64_t state;

    // Devuelve un float uniforme en [minValue, maxValue).
    float next(float minValue, float maxValue) {
        state = splitMix64(state);
        const float unit = static_cast<float>(state >> 40) * (1.0f / 16777216.0f);
        return minValue + (maxValue - minValue) * unit;
    }
};

// Crea el estado inicial de la particula "index" para la explosion del slot.
inline void initParticle(Particle& particle, int index, int slotIndex,
                         const FireworkSlot& slot, std::uint64_t seed) {
    ParticleRandom random{seed ^ splitMix64(slot.launchId) ^
                          (static_cast<std::uint64_t>(index) * 0xD1B54A32D192ED03ull)};

    // Direccion y rapidez inicial (trigonometria para descomponer la velocidad).
    const float angle = random.next(0.0f, 2.0f * PI);
    const float speed = slot.power * std::sqrt(random.next(0.05f, 1.0f));

    particle.x = slot.centerX;
    particle.y = slot.centerY;
    particle.vx = std::cos(angle) * speed;
    particle.vy = std::sin(angle) * speed;

    // Color: variacion alrededor del color principal del fuego.
    // Algunas particulas (~8%) salen blancas como chispas.
    if (random.next(0.0f, 1.0f) < 0.08f) {
        particle.r = 1.0f;
        particle.g = 1.0f;
        particle.b = 1.0f;
    } else {
        particle.r = std::fmin(1.0f, slot.colorR * random.next(0.75f, 1.15f));
        particle.g = std::fmin(1.0f, slot.colorG * random.next(0.75f, 1.15f));
        particle.b = std::fmin(1.0f, slot.colorB * random.next(0.75f, 1.15f));
    }

    particle.life = 1.0f;
    particle.decay = random.next(0.30f, 0.60f);
    particle.phase = random.next(0.0f, 2.0f * PI);
    particle.slot = slotIndex;
}

// Avanza una particula un paso de tiempo. Devuelve true si sigue viva.
// dragFactor = exp(-AIR_DRAG * dt) se calcula una vez por frame.
inline bool updateParticle(Particle& particle, float deltaTime, float time,
                           float dragFactor, float halfWidth) {
    if (particle.life <= 0.0f) {
        return false;
    }

    // Gravedad: v = v0 + g * dt
    particle.vy += GRAVITY * deltaTime;

    // Viento oscilante: depende del tiempo, la altura y la fase de la particula.
    const float wind = WIND_STRENGTH *
                       std::sin(WIND_FREQUENCY * time + particle.y * WIND_WAVE + particle.phase);
    particle.vx += wind * deltaTime;

    // Resistencia del aire (decaimiento exponencial de la velocidad).
    particle.vx *= dragFactor;
    particle.vy *= dragFactor;

    // Posicion: p = p0 + v * dt
    particle.x += particle.vx * deltaTime;
    particle.y += particle.vy * deltaTime;

    // Rebote contra el suelo con perdida de energia.
    if (particle.y < WORLD_BOTTOM) {
        particle.y = WORLD_BOTTOM;
        particle.vy = -particle.vy * RESTITUTION;
        particle.vx *= FLOOR_FRICTION;
    }

    // Rebote contra las paredes laterales.
    if (particle.x < -halfWidth) {
        particle.x = -halfWidth;
        particle.vx = -particle.vx * RESTITUTION;
    } else if (particle.x > halfWidth) {
        particle.x = halfWidth;
        particle.vx = -particle.vx * RESTITUTION;
    }

    // Desvanecimiento.
    particle.life -= particle.decay * deltaTime;
    if (particle.life <= 0.0f) {
        particle.life = 0.0f;
        return false;
    }
    return true;
}

// Escribe el vertice correspondiente a la particula (mismo indice).
// Las particulas apagadas se mandan fuera de pantalla con alfa 0.
inline void writeVertex(const Particle& particle, Vertex& vertex, float time) {
    if (particle.life <= 0.0f) {
        vertex = {HIDDEN_POSITION, HIDDEN_POSITION, 0.0f, 0.0f, 0.0f, 0.0f};
        return;
    }

    // Parpadeo con seno y un "calor" que blanquea las particulas recien creadas.
    const float twinkle = 0.75f + 0.25f * std::sin(TWINKLE_FREQUENCY * time + particle.phase);
    const float heat = particle.life * particle.life * 0.35f;

    vertex.x = particle.x;
    vertex.y = particle.y;
    vertex.r = std::fmin(1.0f, (particle.r + heat) * twinkle);
    vertex.g = std::fmin(1.0f, (particle.g + heat) * twinkle);
    vertex.b = std::fmin(1.0f, (particle.b + heat) * twinkle);
    vertex.a = particle.life;
}

}  // namespace physics
