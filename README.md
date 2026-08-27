# Proyecto #1 - Screensaver de fuegos artificiales con OpenGL

Prueba de concepto correspondiente a la **Entrega 2** del curso de Computacion Paralela y Distribuida, Universidad del Valle de Guatemala, semestre 2 de 2026.

## Integrantes

- Adrian Gonzalez - 23152
- Fernando Mendoza - 19644
- Hansel Lopez - 19026

## Alcance de esta entrega

Esta version demuestra el uso de **C++ + OpenGL** para crear una ventana grafica y renderizar el elemento principal definido en la propuesta: una explosion de `N` particulas de colores que se dispersan, son afectadas por gravedad y se desvanecen.

La implementacion es **secuencial**. OpenMP se incorporara en una entrega posterior, principalmente en el ciclo de actualizacion independiente de las particulas.

## Caracteristicas

- Ventana OpenGL de 800x600.
- OpenGL 3.3 Core Profile.
- GLFW para ventana, contexto y eventos.
- GLEW para cargar funciones de OpenGL.
- `N` configurable por linea de comandos.
- Colores pseudoaleatorios.
- Movimiento y gravedad.
- Desvanecimiento de particulas.
- Regeneracion automatica de explosiones.
- FPS y valor de `N` visibles en el titulo de la ventana.
- Validacion defensiva del argumento de entrada.

## Dependencias

- Compilador compatible con C++17.
- CMake 3.16 o superior.
- OpenGL.
- GLFW 3.3 o superior.
- GLEW.

### Ubuntu / Debian / WSL2

```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglew-dev libgl1-mesa-dev
```

> En WSL2 se requiere soporte de aplicaciones graficas. En Windows 11 con WSLg normalmente funciona de forma integrada.

## Compilacion

Desde la raiz del proyecto:

```bash
cmake -S . -B build
cmake --build build
```

## Ejecucion

```bash
./build/screensaver <N>
```

Ejemplo:

```bash
./build/screensaver 500
```

Si no se proporciona `N`, el programa utiliza 500 particulas.

Valores validos:

```text
1 <= N <= 200000
```

Presione `ESC` o cierre la ventana para finalizar.

## Estructura

```text
entrega2_opengl_screensaver/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── src/
│   └── main.cpp
└── docs/
    ├── Entrega_2_OpenGL.docx
    ├── Entrega_2_OpenGL.pdf
    └── Entrega_2_OpenGL.md
```

## Preparacion para OpenMP

El metodo `updateParticles()` mantiene la actualizacion de cada particula independiente de las demas:

```cpp
for (Particle& particle : particles) {
    // actualizar estado individual
}
```

Esto deja preparado el programa para evaluar posteriormente una paralelizacion con OpenMP. El renderizado OpenGL se mantiene separado de la actualizacion fisica.

## Referencias

- Khronos Group. OpenGL: https://www.khronos.org/opengl/
- GLFW Documentation: https://www.glfw.org/documentation.html
- LearnOpenGL - Hello Window: https://learnopengl.com/Getting-started/Hello-Window
