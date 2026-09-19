# Proyecto #1 - Screensaver de fuegos artificiales con OpenGL y OpenMP

Proyecto final del curso de Computacion Paralela y Distribuida, Universidad del Valle de Guatemala, semestre 2 de 2026.

## Integrantes

- Adrian Gonzalez - 23152
- Fernando Mendoza - 19644
- Hansel Lopez - 19026

## Descripcion

Screensaver que simula varios fuegos artificiales al mismo tiempo. Cada explosion lanza particulas de colores pseudoaleatorios que se mueven con gravedad, resistencia del aire y viento oscilante, rebotan contra el suelo y las paredes, parpadean y se desvanecen. Cuando un fuego se apaga, se vuelve a lanzar en otra posicion con otro color.

El programa tiene tres versiones del calculo de cada frame:

| Version | Descripcion |
|---------|-------------|
| `seq`   | Secuencial, sin OpenMP. Es la linea base del speedup. |
| `par1`  | Primera version paralela: un `#pragma omp parallel for` por cada fase del frame. |
| `par2`  | Version mejorada: una sola region paralela por frame con `omp for`, `omp single`, `reduction` y `nowait`. |

Las tres usan exactamente las mismas funciones por particula (`src/physics.hpp`), asi que producen el mismo resultado; en el benchmark se verifica comparando un checksum del estado final.

## Dependencias

- Compilador con soporte de C++17 y OpenMP (GCC 9+ o Clang con libomp).
- CMake 3.16 o superior.
- OpenGL 3.3, GLFW 3.3+ y GLEW.
- (Opcional, para graficas) Python 3 con `pandas`, `matplotlib` y `tabulate`.

### Ubuntu / Debian / WSL2

```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglew-dev libgl1-mesa-dev
pip install pandas matplotlib tabulate
```

## Compilacion

```bash
cmake -S . -B build
cmake --build build
```

Por defecto se compila en modo `Release` para que las mediciones sean representativas.

## Ejecucion

```bash
./build/screensaver [N] [opciones]
```

`N` es la cantidad total de particulas (1 a 2,000,000). Si no se indica se usan 5000.

| Opcion | Descripcion | Defecto |
|--------|-------------|---------|
| `--mode seq\|par1\|par2` | Version a ejecutar | `par2` |
| `--threads T` | Hilos OpenMP (1-256) | todos los del equipo |
| `--fireworks E` | Fuegos simultaneos (1-64, no mayor que N) | 6 |
| `--width W` / `--height H` | Tamano de ventana (min 640x480) | 800x600 |
| `--size S` | Tamano de particula en pixeles (1-20) | 4 |
| `--seed S` | Semilla pseudoaleatoria | aleatoria |
| `--no-vsync` | Desactiva vsync para ver los FPS maximos | vsync activo |
| `--help` | Muestra la ayuda | |

Ejemplos:

```bash
./build/screensaver 20000
./build/screensaver 200000 --mode seq --no-vsync
./build/screensaver 200000 --mode par2 --threads 8 --no-vsync
```

Durante la ejecucion se puede cambiar de version con las teclas `1` (seq), `2` (par1) y `3` (par2). `ESC` cierra el programa. Los FPS y el tiempo de simulacion se muestran en el titulo de la ventana y cada segundo en la consola.

## Benchmark (speedup y eficiencia)

```bash
./build/screensaver 200000 --benchmark --thread-list 2,4,8
```

El modo benchmark:

1. Desactiva vsync.
2. Usa un paso de tiempo fijo y la misma semilla, de modo que todas las versiones simulan exactamente lo mismo.
3. Ejecuta `seq` y luego `par1` y `par2` con cada cantidad de hilos, 10 mediciones por prueba (`--runs`) de 300 frames cada una (`--frames`).
4. Mide con `omp_get_wtime()` el tiempo de simulacion por separado del tiempo total del frame.
5. Imprime cada medicion y una tabla con speedup (`T_seq / T_par`, usando el promedio) y eficiencia (`speedup / hilos`).
6. Agrega las mediciones a `results/benchmark.csv` y el resumen a `results/benchmark_resumen.csv`.

Con `--sim-only` se mide solo la simulacion sin abrir ventana.

Para correr todas las pruebas con varios valores de N y generar las graficas:

```bash
./scripts/run_benchmarks.sh
python3 scripts/analyze_results.py
```

## Estructura

```text
.
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp               # flujo principal y ciclo del screensaver
│   ├── config.hpp/.cpp        # lectura y validacion de argumentos
│   ├── simulation.hpp         # estructuras y API de la simulacion
│   ├── physics.hpp            # fisica por particula (compartida por las 3 versiones)
│   ├── simulation_common.cpp  # inicializacion, planificacion de lanzamientos, checksum
│   ├── simulation_seq.cpp     # version secuencial
│   ├── simulation_par.cpp     # versiones paralelas par1 y par2
│   ├── renderer.hpp/.cpp      # ventana y dibujo con OpenGL (RAII)
│   └── benchmark.hpp/.cpp     # mediciones, speedup, eficiencia y CSV
├── scripts/
│   ├── run_benchmarks.sh      # benchmark para varios N
│   └── analyze_results.py     # graficas y tabla para el informe
└── results/                   # salidas del benchmark
```

## Paralelizacion

- **Particion:** cada particula es una tarea independiente (descomposicion de dominio).
- **Comunicacion:** las particulas no dependen entre si. Lo unico compartido es el conteo de particulas vivas por fuego, que se resuelve con `reduction(+ : aliveCounts[:E])`.
- **Aglomeracion y mapeo:** `schedule(static)` asigna bloques contiguos a cada hilo, lo que aprovecha la cache y evita false sharing al escribir `particles[i]` y `vertices[i]`.
- **Sincronizacion:** barreras implicitas de `omp for`, `omp single` para la parte secuencial (usa el generador de numeros compartido) y `nowait` donde los rangos no se traslapan.
- **Numeros aleatorios:** cada particula genera sus valores con SplitMix64 a partir de (semilla, explosion, indice), sin estado compartido, asi que no hay condiciones de carrera.
- **Render:** todas las llamadas a OpenGL quedan en el hilo principal, fuera de las regiones paralelas, porque el contexto grafico no es thread-safe.

## Referencias

- OpenMP Architecture Review Board. OpenMP API Specification 5.2: https://www.openmp.org/specifications/
- Khronos Group. OpenGL: https://www.khronos.org/opengl/
- GLFW Documentation: https://www.glfw.org/documentation.html
- LearnOpenGL - Hello Window: https://learnopengl.com/Getting-started/Hello-Window
- Foster, I. *Designing and Building Parallel Programs* (metodo PCAM).
