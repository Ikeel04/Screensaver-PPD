#!/usr/bin/env bash
# Ejecuta el benchmark completo para varios valores de N.
# Cada ejecucion mide seq, par1 y par2 (10 mediciones por prueba) y
# agrega los resultados a results/benchmark.csv y results/benchmark_resumen.csv.
#
# Uso:
#   ./scripts/run_benchmarks.sh                 # con ventana (render incluido)
#   ./scripts/run_benchmarks.sh --sim-only      # solo simulacion, sin ventana
#   THREADS=2,4,8 ./scripts/run_benchmarks.sh   # elegir hilos a probar

set -euo pipefail

BINARY="./build/screensaver"
VALUES_OF_N=(10000 50000 100000 200000 500000 1000000)
RUNS=10
FRAMES=300
EXTRA_ARGS=("$@")

if [[ ! -x "$BINARY" ]]; then
    echo "No se encontro $BINARY. Compile primero:"
    echo "  cmake -S . -B build && cmake --build build"
    exit 1
fi

mkdir -p results
THREAD_ARGS=()
if [[ -n "${THREADS:-}" ]]; then
    THREAD_ARGS=(--thread-list "$THREADS")
fi

for N in "${VALUES_OF_N[@]}"; do
    echo "=============================================="
    echo " N = $N"
    echo "=============================================="
    "$BINARY" "$N" --benchmark --runs "$RUNS" --frames "$FRAMES" \
        --csv results/benchmark.csv ${THREAD_ARGS[@]+"${THREAD_ARGS[@]}"} ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"} \
        | tee "results/log_N${N}.txt"
done

echo
echo "Listo. Para generar las graficas:"
echo "  python3 scripts/analyze_results.py"
