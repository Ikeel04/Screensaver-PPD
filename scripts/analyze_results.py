"""Genera tablas y graficas a partir de results/benchmark_resumen.csv.

Uso:
    python3 scripts/analyze_results.py [ruta_resumen.csv]

Requiere: pandas y matplotlib (pip install pandas matplotlib)
Salidas en results/:
    speedup_vs_hilos.png, eficiencia_vs_hilos.png,
    fps_vs_n.png, tiempo_sim_vs_n.png, tabla_resumen.md
"""

import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

RESULTS_DIR = Path("results")


def load_summary(path: Path) -> pd.DataFrame:
    if not path.exists():
        sys.exit(f"No existe {path}. Ejecute primero scripts/run_benchmarks.sh")
    data = pd.read_csv(path)
    # Si una misma prueba se corrio varias veces, se queda la ultima.
    data = data.drop_duplicates(subset=["N", "render", "modo", "hilos"], keep="last")
    return data.sort_values(["render", "N", "modo", "hilos"])


def plot_metric_vs_threads(data: pd.DataFrame, column: str, ylabel: str, filename: str, ideal: bool):
    for render, group in data[data["modo"] != "seq"].groupby("render"):
        largest_n = group["N"].max()
        subset = group[group["N"] == largest_n]
        fig, ax = plt.subplots(figsize=(7, 4.5))
        for mode, mode_data in subset.groupby("modo"):
            ax.plot(mode_data["hilos"], mode_data[column], marker="o", label=mode)
        threads = sorted(subset["hilos"].unique())
        if ideal:
            ax.plot(threads, threads, linestyle="--", color="gray", label="ideal")
        else:
            ax.axhline(1.0, linestyle="--", color="gray", label="ideal")
        ax.set_xlabel("Hilos")
        ax.set_ylabel(ylabel)
        ax.set_title(f"{ylabel} vs hilos (N = {largest_n:,}, render = {'si' if render else 'no'})")
        ax.set_xticks(threads)
        ax.grid(alpha=0.3)
        ax.legend()
        fig.tight_layout()
        suffix = "_render" if render else "_simonly"
        fig.savefig(RESULTS_DIR / filename.replace(".png", f"{suffix}.png"), dpi=150)
        plt.close(fig)


def plot_metric_vs_n(data: pd.DataFrame, column: str, ylabel: str, filename: str):
    for render, group in data.groupby("render"):
        max_threads = group[group["modo"] != "seq"]["hilos"].max()
        fig, ax = plt.subplots(figsize=(7, 4.5))
        for mode in ["seq", "par1", "par2"]:
            mode_data = group[group["modo"] == mode]
            if mode != "seq":
                mode_data = mode_data[mode_data["hilos"] == max_threads]
            label = mode if mode == "seq" else f"{mode} ({max_threads} hilos)"
            ax.plot(mode_data["N"], mode_data[column], marker="o", label=label)
        if column == "fps_promedio":
            ax.axhline(60, linestyle="--", color="red", label="60 FPS")
        ax.set_xscale("log")
        ax.set_xlabel("N (particulas)")
        ax.set_ylabel(ylabel)
        ax.set_title(f"{ylabel} vs N (render = {'si' if render else 'no'})")
        ax.grid(alpha=0.3, which="both")
        ax.legend()
        fig.tight_layout()
        suffix = "_render" if render else "_simonly"
        fig.savefig(RESULTS_DIR / filename.replace(".png", f"{suffix}.png"), dpi=150)
        plt.close(fig)


def write_markdown_table(data: pd.DataFrame):
    columns = ["N", "render", "modo", "hilos", "sim_ms_promedio", "sim_ms_desv",
               "speedup", "eficiencia", "fps_promedio", "speedup_fps"]
    table = data[columns].copy()
    table["render"] = table["render"].map({1: "si", 0: "no"})
    output = RESULTS_DIR / "tabla_resumen.md"
    output.write_text(table.to_markdown(index=False, floatfmt=".3f"), encoding="utf-8")
    print(f"Tabla guardada en {output}")


def main():
    summary_path = Path(sys.argv[1]) if len(sys.argv) > 1 else RESULTS_DIR / "benchmark_resumen.csv"
    data = load_summary(summary_path)

    plot_metric_vs_threads(data, "speedup", "Speedup", "speedup_vs_hilos.png", ideal=True)
    plot_metric_vs_threads(data, "eficiencia", "Eficiencia", "eficiencia_vs_hilos.png", ideal=False)
    plot_metric_vs_n(data, "fps_promedio", "FPS promedio", "fps_vs_n.png")
    plot_metric_vs_n(data, "sim_ms_promedio", "Tiempo de simulacion (ms/frame)", "tiempo_sim_vs_n.png")

    try:
        write_markdown_table(data)
    except ImportError:
        print("Instale 'tabulate' para exportar la tabla en Markdown (pip install tabulate).")

    print("Graficas guardadas en results/")


if __name__ == "__main__":
    main()
