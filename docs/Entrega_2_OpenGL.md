# Entrega 2 - Proyecto #1: Screensaver de fuegos artificiales con OpenGL

**Universidad del Valle de Guatemala**  
**Computacion Paralela y Distribuida - 2026**

**Integrantes:**
- Adrian Gonzalez - 23152
- Fernando Mendoza - 19644
- Hansel Lopez - 19026

## 1. Investigacion y seleccion de herramienta grafica

### 1.1 Herramienta seleccionada: OpenGL

Para el desarrollo del screensaver se selecciono **OpenGL (Open Graphics Library)** como herramienta principal de renderizado. OpenGL es una API grafica multiplataforma orientada a la generacion de graficos 2D y 3D y puede aprovechar aceleracion por hardware.

Se utilizara **GLFW** como biblioteca auxiliar para crear la ventana, establecer el contexto de OpenGL y procesar eventos basicos. Tambien se utiliza **GLEW (OpenGL Extension Wrangler Library)** para cargar las funciones de OpenGL moderno de forma portable en el entorno elegido.

La configuracion seleccionada para el proyecto es:

- Lenguaje: C++17.
- Renderizado: OpenGL.
- Ventana y contexto grafico: GLFW.
- Carga de funciones OpenGL: GLEW.
- Version objetivo: OpenGL 3.3 Core Profile.

### 1.2 Justificacion

La propuesta de la Entrega 1 define un screensaver que simula fuegos artificiales. Cada explosion genera `N` particulas con posicion, velocidad, color y tiempo de vida; las particulas se dispersan, reciben el efecto de la gravedad y se desvanecen. Esta estructura se adapta bien al renderizado mediante OpenGL.

OpenGL resulta apropiado porque separa con claridad el calculo de la simulacion en CPU del renderizado grafico. Para este proyecto, la CPU puede actualizar posicion, velocidad, gravedad y vida de las particulas, mientras OpenGL dibuja el estado resultante. Esa separacion tambien favorece la etapa posterior del proyecto, cuando la actualizacion independiente de las particulas se paralelice con OpenMP.

Adicionalmente, OpenGL es multiplataforma, tiene amplia documentacion y permite manejar eficientemente una cantidad elevada de elementos graficos.

### 1.3 Requisitos basicos para instalacion y uso con C/C++

Se requieren los siguientes componentes:

- Compilador compatible con C++17.
- CMake 3.16 o superior.
- OpenGL.
- GLFW 3.3 o superior.
- GLEW.

En Ubuntu, Debian o WSL2 se pueden instalar con:

```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglew-dev libgl1-mesa-dev
```

Para compilar:

```bash
cmake -S . -B build
cmake --build build
```

Para ejecutar con 500 particulas:

```bash
./build/screensaver 500
```

### 1.4 Ejemplo de referencia

Un ejemplo de referencia para una aplicacion OpenGL es la creacion de una ventana con GLFW y la ejecucion de un ciclo de renderizado. El flujo general consiste en inicializar GLFW, crear una ventana y contexto, cargar las funciones de OpenGL, ejecutar el ciclo de dibujo, intercambiar buffers y procesar eventos.

Este patron aparece en la documentacion de GLFW y en el tutorial **Hello Window** de LearnOpenGL, y constituye la base tecnica utilizada por la prueba de concepto.

## 2. Prueba de concepto del screensaver

### 2.1 Objetivo

La prueba de concepto busca comprobar que el equipo puede:

- Crear una ventana grafica con GLFW y OpenGL.
- Inicializar el pipeline de renderizado.
- Generar dinamicamente `N` particulas.
- Mostrar particulas de distintos colores.
- Animar las particulas en tiempo real.
- Aplicar gravedad.
- Mostrar los FPS actuales y el valor de `N`.
- Mantener la estructura preparada para paralelizacion posterior.

Esta implementacion corresponde todavia a una version secuencial inicial.

### 2.2 Relacion con la propuesta de la Entrega 1

La propuesta original describe un espectaculo de fuegos artificiales. Cada explosion genera `N` particulas con una posicion y velocidad iniciales. En cada frame se actualiza la velocidad vertical aplicando gravedad, luego se modifica la posicion y finalmente se reduce la vida de la particula hasta que desaparece.

El movimiento implementado sigue una aproximacion discreta de:

```text
vy = vy + g * dt
x  = x + vx * dt
y  = y + vy * dt
```

Cuando todas las particulas de una explosion desaparecen, se genera automaticamente una nueva explosion en una posicion pseudoaleatoria.

### 2.3 Creacion de la ventana grafica

La prueba de concepto crea una ventana de **800 x 600 pixeles**, superior al minimo de 640 x 480 establecido en las instrucciones generales del proyecto.

La aplicacion utiliza OpenGL 3.3 Core Profile y permanece activa hasta que el usuario presiona `ESC` o cierra la ventana.

### 2.4 Elemento visual implementado

El elemento implementado es la **explosion de particulas del fuego artificial**. Cada explosion genera exactamente `N` particulas.

Para cada particula se generan pseudoaleatoriamente:

- direccion inicial;
- velocidad inicial;
- componentes rojo, verde y azul;
- tiempo de vida.

Las particulas se dibujan como puntos circulares sobre un fondo oscuro. El fragment shader utiliza `gl_PointCoord` para descartar los fragmentos fuera del radio, produciendo una particula circular en lugar de un cuadrado.

### 2.5 Movimiento y fisica

Durante cada frame:

1. Se aplica gravedad a la velocidad vertical.
2. Se actualiza la posicion horizontal.
3. Se actualiza la posicion vertical.
4. Se reduce el tiempo de vida.
5. Se reduce la opacidad visual.
6. Las particulas apagadas dejan de renderizarse.

El resultado produce trayectorias similares a parabolas, cumpliendo desde esta etapa con el componente de fisica solicitado para el screensaver.

### 2.6 Parametro N

El parametro `N` representa la cantidad de particulas generadas por cada explosion y puede proporcionarse desde linea de comandos:

```bash
./build/screensaver 500
```

La aplicacion valida defensivamente el argumento. El rango definido para la prueba de concepto es:

```text
1 <= N <= 200000
```

Si no se proporciona un valor, se utilizan 500 particulas.

### 2.7 Colores pseudoaleatorios

Cada particula recibe componentes RGB pseudoaleatorios. Se evita utilizar colores demasiado oscuros para que las particulas sean visibles sobre el fondo.

### 2.8 Frames por segundo

La aplicacion calcula los FPS de forma periodica y los muestra junto con el valor de `N` en el titulo de la ventana, por ejemplo:

```text
OpenGL Fireworks | N = 500 | FPS = 60.0
```

Esto permite comenzar a observar el comportamiento del programa cuando se modifica el numero de particulas y deja preparada la instrumentacion necesaria para pruebas de rendimiento posteriores.

### 2.9 Preparacion para paralelizacion

La Entrega 2 mantiene el programa secuencial. La actualizacion de particulas se encuentra aislada dentro de `updateParticles()`. Cada particula modifica unicamente su propio estado, por lo que este ciclo constituye un candidato natural para utilizar posteriormente `#pragma omp parallel for`.

El renderizado con OpenGL permanece fuera de esa futura seccion paralela, manteniendo separado el acceso al contexto grafico de los calculos fisicos.

## 3. Resultado de la prueba de concepto

La prueba de concepto cubre los objetivos de esta entrega:

- seleccion definitiva de OpenGL;
- descripcion y justificacion de la herramienta;
- requisitos basicos de instalacion y uso;
- referencia de una aplicacion grafica;
- ventana OpenGL funcional de 800 x 600;
- visualizacion de `N` particulas;
- colores pseudoaleatorios;
- movimiento y gravedad;
- desvanecimiento de particulas;
- nuevas explosiones durante la ejecucion;
- FPS visibles;
- estructura preparada para incorporar OpenMP posteriormente.

No se incorpora aun la paralelizacion, el calculo de speedup o eficiencia, ni la implementacion completa de todas las caracteristicas previstas para la entrega final del proyecto.

## Referencias

1. Universidad del Valle de Guatemala. *Proyecto #1 - Computacion Paralela y Distribuida, Semestre 2, 2026*.
2. Gonzalez, A.; Mendoza, F.; Lopez, H. *Propuesta - Proyecto #1: Screensaver paralelo con OpenMP*, 2026.
3. Khronos Group. *OpenGL*. https://www.khronos.org/opengl/
4. GLFW Project. *GLFW Documentation*. https://www.glfw.org/documentation.html
5. de Vries, J. *LearnOpenGL - Hello Window*. https://learnopengl.com/Getting-started/Hello-Window
