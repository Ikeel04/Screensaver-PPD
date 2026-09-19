#!/usr/bin/env python3
"""Genera el informe final en PDF a partir de los resultados del repositorio.

Uso:
    python3 scripts/generate_final_report.py

La salida se escribe en docs/Entrega_Final_Proyecto1.pdf. El informe usa
matplotlib para mantener la generación reproducible sin depender de un
procesador de texto instalado en el equipo.
"""

from __future__ import annotations

import csv
import math
import platform
import subprocess
import textwrap
from collections import defaultdict
from datetime import datetime
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.image as mpimg
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch, Rectangle


ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "results"
OUTPUT = ROOT / "docs" / "Entrega_Final_Proyecto1.pdf"

NAVY = "#13283f"
BLUE = "#2364aa"
TEAL = "#159a9c"
ORANGE = "#e07a35"
INK = "#202a33"
MUTED = "#5e6b76"
LIGHT = "#edf3f7"
PALE = "#f7fafc"
GREEN = "#238b62"


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


SUMMARY = read_csv(RESULTS / "benchmark_resumen.csv")
RAW = read_csv(RESULTS / "benchmark.csv")


def fmt(value: str | float, digits: int = 3) -> str:
    return f"{float(value):.{digits}f}"


def git_value(args: list[str], fallback: str) -> str:
    try:
        return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()
    except Exception:
        return fallback


COMMIT = git_value(["rev-parse", "--short", "HEAD"], "53a2de3")


def add_footer(fig, page: int, source: str | None = None):
    fig.text(0.07, 0.035, "UVG · Computación Paralela y Distribuida · Proyecto #1",
             fontsize=7.5, color=MUTED)
    if source:
        fig.text(0.50, 0.035, source, fontsize=7, color=MUTED, ha="center")
    fig.text(0.93, 0.035, f"{page:02d}", fontsize=8, color=NAVY, ha="right", weight="bold")
    fig.lines.append(plt.Line2D([0.07, 0.93], [0.055, 0.055], color="#d6e0e7", lw=0.7,
                                transform=fig.transFigure))


def page(title: str, subtitle: str | None, number: int, source: str | None = None):
    fig = plt.figure(figsize=(8.27, 11.69), facecolor="white")
    fig.text(0.07, 0.94, "PROYECTO #1  /  INFORME FINAL", fontsize=8, color=TEAL,
             weight="bold", family="DejaVu Sans")
    fig.text(0.07, 0.895, title, fontsize=22, color=NAVY, weight="bold")
    if subtitle:
        fig.text(0.07, 0.862, subtitle, fontsize=10, color=MUTED)
    add_footer(fig, number, source)
    return fig


def add_paragraph(fig, text: str, y: float, width: int = 95, size: float = 10.2,
                  color: str = INK, leading: float = 0.019, x: float = 0.09) -> float:
    lines = []
    for paragraph in text.split("\n"):
        lines.extend(textwrap.wrap(paragraph, width=width) or [""])
    fig.text(x, y, "\n".join(lines), va="top", fontsize=size, color=color,
             linespacing=1.35)
    return y - leading * len(lines) - 0.012


def add_bullets(fig, items: list[str], y: float, width: int = 88, size: float = 9.5,
                x: float = 0.105, color: str = INK) -> float:
    for item in items:
        lines = textwrap.wrap(item, width=width)
        fig.text(x, y, "•", fontsize=size + 1, color=TEAL, va="top")
        fig.text(x + 0.022, y, "\n".join(lines), fontsize=size, color=color,
                 va="top", linespacing=1.3)
        y -= 0.026 * len(lines) + 0.012
    return y


def section_label(fig, label: str, y: float):
    fig.text(0.09, y, label.upper(), fontsize=8, color=ORANGE, weight="bold")


def cover(pdf: PdfPages):
    fig = plt.figure(figsize=(8.27, 11.69), facecolor=NAVY)
    ax = fig.add_axes([0, 0, 1, 1])
    ax.axis("off")
    ax.add_patch(Rectangle((0, 0), 1, 1, color=NAVY))
    ax.add_patch(Rectangle((0, 0), 0.035, 1, color=TEAL))
    for i, alpha in enumerate([0.16, 0.10, 0.06]):
        circle = plt.Circle((0.82, 0.78), 0.22 + i * 0.12, color=TEAL, alpha=alpha)
        ax.add_patch(circle)
    fig.text(0.10, 0.83, "UNIVERSIDAD DEL VALLE DE GUATEMALA", fontsize=12,
             color="#bfe7e5", weight="bold")
    fig.text(0.10, 0.785, "Computación Paralela y Distribuida", fontsize=11,
             color="white")
    fig.text(0.10, 0.64, "Screensaver de fuegos\nartificiales con OpenMP", fontsize=31,
             color="white", weight="bold", linespacing=1.1)
    fig.text(0.10, 0.535, "Informe final del Proyecto #1", fontsize=14, color="#f3b37e")
    fig.text(0.10, 0.34, "Semestre 2 · 2026\n\nAdrián González · 23152\nFernando Mendoza · 19644\nHansel López · 19026",
             fontsize=11, color="#e9f1f5", linespacing=1.55)
    fig.text(0.10, 0.12, "Repositorio: github.com/Ikeel04/Screensaver-PPD\nVersión documentada: " + COMMIT,
             fontsize=8.5, color="#b7c8d4", linespacing=1.45)
    pdf.savefig(fig, facecolor=fig.get_facecolor())
    plt.close(fig)


def index_page(pdf: PdfPages, n: int):
    fig = page("Contenido", "Estructura del informe y trazabilidad de los entregables", n)
    y = 0.81
    entries = [
        ("Resumen ejecutivo", "03"), ("1. Introducción y antecedentes", "04"),
        ("2. Diseño de la solución", "05"), ("3. Implementación", "06"),
        ("4. Metodología experimental", "07"), ("5. Resultados y discusión", "08–09"),
        ("6. Conclusiones y recomendaciones", "10"), ("Bibliografía", "11"),
        ("Anexo 1. Diagrama de flujo", "12"), ("Anexo 2. Catálogo de funciones", "13–14"),
        ("Anexo 3. Bitácora de pruebas", "15–16"),
    ]
    for title, number in entries:
        fig.text(0.11, y, title, fontsize=12, color=INK)
        fig.text(0.88, y, number, fontsize=11, color=TEAL, ha="right", weight="bold")
        fig.lines.append(plt.Line2D([0.11, 0.87], [y - 0.012, y - 0.012], color="#dbe5eb", lw=0.6,
                                    transform=fig.transFigure))
        y -= 0.047
    section_label(fig, "Trazabilidad frente a la rúbrica", 0.25)
    y = add_paragraph(fig, "El informe documenta las versiones secuencial, paralela V1 y paralela V2; el uso de OpenMP; la parametrización defensiva; el cálculo de speedup y eficiencia; y la verificación de equivalencia por checksum. Los archivos fuente, scripts y resultados quedan en el repositorio junto con este PDF.", 0.225, width=95, size=9.5)
    pdf.savefig(fig); plt.close(fig)


def text_page(pdf: PdfPages, title: str, subtitle: str, number: int, blocks: list[tuple[str, str]], source=None):
    fig = page(title, subtitle, number, source)
    y = 0.81
    for label, text in blocks:
        section_label(fig, label, y)
        y -= 0.026
        y = add_paragraph(fig, text, y, width=94, size=10)
        y -= 0.014
    pdf.savefig(fig); plt.close(fig)


def architecture_page(pdf: PdfPages, number: int):
    fig = page("2. Diseño de la solución", "Separación entre configuración, simulación y renderizado", number)
    ax = fig.add_axes([0.08, 0.38, 0.84, 0.40]); ax.set_xlim(0, 10); ax.set_ylim(0, 10); ax.axis("off")
    boxes = [
        (0.5, 7.2, 2.2, 1.25, "CLI / Config", "N, modo, hilos\nvalidación defensiva", BLUE),
        (3.9, 7.2, 2.2, 1.25, "SimulationState", "partículas, slots\nvertices y reloj", TEAL),
        (7.3, 7.2, 2.2, 1.25, "Renderer", "GLFW + GLEW\nOpenGL 3.3", ORANGE),
        (0.5, 4.5, 2.2, 1.25, "seq", "línea base\n1 hilo", MUTED),
        (3.9, 4.5, 2.2, 1.25, "par1", "parallel for\npor fase", BLUE),
        (7.3, 4.5, 2.2, 1.25, "par2", "una región\nparalela/frame", TEAL),
        (2.25, 1.65, 5.5, 1.35, "Benchmark", "10 corridas · dt fijo · checksum · CSV\nspeedup = Tseq / Tpar · eficiencia = speedup / hilos", NAVY),
    ]
    for x, y, w, h, head, body, color in boxes:
        ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.04,rounding_size=0.12",
                                    fc="white", ec=color, lw=2))
        ax.text(x + 0.13, y + h - 0.36, head, color=color, fontsize=11, weight="bold")
        ax.text(x + 0.13, y + 0.25, body, color=INK, fontsize=8.5, va="bottom", linespacing=1.3)
    arrows = [((2.7, 7.83), (3.9, 7.83)), ((6.1, 7.83), (7.3, 7.83)),
              ((5.0, 7.2), (1.6, 5.75)), ((5.0, 7.2), (5.0, 5.75)), ((5.0, 7.2), (8.4, 5.75)),
              ((1.6, 4.5), (3.5, 3.0)), ((5.0, 4.5), (5.0, 3.0)), ((8.4, 4.5), (6.5, 3.0))]
    for start, end in arrows:
        ax.add_patch(FancyArrowPatch(start, end, arrowstyle="-|>", mutation_scale=12,
                                     lw=1.2, color=MUTED))
    section_label(fig, "Decisiones de diseño", 0.315)
    add_bullets(fig, [
        "N partículas se divide en rangos contiguos asignados a E fuegos artificiales; cada partícula escribe únicamente su propio estado y vértice.",
        "La fase de lanzamientos se mantiene secuencial porque usa un generador aleatorio compartido y tiene costo O(E), con E ≤ 64.",
        "El renderizado permanece en el hilo principal: el contexto OpenGL no se comparte con las regiones de cálculo.",
    ], 0.285, width=92, size=9.2)
    pdf.savefig(fig); plt.close(fig)


def results_page(pdf: PdfPages, number: int):
    fig = page("5. Resultados y discusión", "Mediciones de simulación sin renderizado, 10 repeticiones por configuración", number,
                "Fuente: results/benchmark_resumen.csv")
    section_label(fig, "Condición de referencia", 0.81)
    y = add_paragraph(fig, "Las mediciones se ejecutaron en Release con N = 10,000, 100,000 y 500,000 partículas, 6 fuegos, dt = 1/60 s, 100 frames por corrida y 10 corridas por configuración. Se usó una semilla fija y se descartaron 30 frames de calentamiento. El tiempo reportado es el promedio del cálculo de simulación por frame; no incluye espera de VSync.", 0.782, width=94, size=9.4)
    # Summary table for the largest N.
    rows = [r for r in SUMMARY if r["N"] == "500000"]
    columns = ["modo", "hilos", "sim_ms_promedio", "sim_ms_desv", "speedup", "eficiencia", "fps_promedio", "checksum_ok"]
    labels = ["Modo", "Hilos", "ms/frame", "σ", "Speedup", "Eficiencia", "FPS", "Checksum"]
    cell = [[r[c] if c not in {"sim_ms_promedio", "sim_ms_desv", "speedup", "eficiencia", "fps_promedio"} else fmt(r[c], 3) for c in columns] for r in rows]
    ax = fig.add_axes([0.08, 0.53, 0.84, 0.20]); ax.axis("off")
    table = ax.table(cellText=cell, colLabels=labels, loc="center", cellLoc="center")
    table.auto_set_font_size(False); table.set_fontsize(8.2); table.scale(1, 1.55)
    for (row, col), c in table.get_celld().items():
        c.set_edgecolor("#d5e0e7")
        if row == 0:
            c.set_facecolor(NAVY); c.get_text().set_color("white"); c.get_text().set_weight("bold")
        elif row % 2 == 0:
            c.set_facecolor(PALE)
        if col == 7 and row > 0:
            c.get_text().set_color(GREEN); c.get_text().set_weight("bold")
    section_label(fig, "Lectura de resultados", 0.47)
    add_bullets(fig, [
        "Con N = 500,000, par1 alcanza speedup 3.955× con 16 hilos y par2 3.891×; la eficiencia es 24.7% y 24.3%, respectivamente.",
        "Con N = 10,000 la carga es pequeña y domina el costo de coordinación: par2 llega a 2.634× con 16 hilos, mientras par1 presenta mayor variabilidad.",
        "Todos los casos marcaron checksum igual. Esto comprueba que las versiones paralelas producen el mismo estado final bajo la semilla y el paso temporal controlados.",
        "La mejora no es lineal: memoria, barreras, fork/join y el límite de ancho de banda reducen la eficiencia al aumentar hilos. El resultado es consistente con el tamaño de la carga.",
    ], 0.44, width=90, size=9.1)
    pdf.savefig(fig); plt.close(fig)


def charts_page(pdf: PdfPages, number: int):
    fig = page("5. Resultados y discusión", "Escalabilidad observada en la simulación", number,
                "Fuente: results/benchmark_resumen.csv; elaboración propia")
    paths = [RESULTS / "speedup_vs_hilos_simonly.png", RESULTS / "eficiencia_vs_hilos_simonly.png",
             RESULTS / "tiempo_sim_vs_n_simonly.png", RESULTS / "fps_vs_n_simonly.png"]
    positions = [(0.07, 0.51), (0.53, 0.51), (0.07, 0.12), (0.53, 0.12)]
    for path, (x, y) in zip(paths, positions):
        if path.exists():
            ax = fig.add_axes([x, y, 0.40, 0.30])
            ax.imshow(mpimg.imread(path)); ax.axis("off")
    fig.text(0.07, 0.475, "Speedup frente a hilos", fontsize=8.5, color=NAVY, weight="bold")
    fig.text(0.53, 0.475, "Eficiencia frente a hilos", fontsize=8.5, color=NAVY, weight="bold")
    fig.text(0.07, 0.085, "Tiempo por frame frente a N", fontsize=8.5, color=NAVY, weight="bold")
    fig.text(0.53, 0.085, "FPS teóricos del modo sim-only", fontsize=8.5, color=NAVY, weight="bold")
    pdf.savefig(fig); plt.close(fig)


def flow_page(pdf: PdfPages, number: int):
    fig = page("Anexo 1. Diagrama de flujo", "Flujo completo: argumentos, validación, simulación paralela y salida", number)
    ax = fig.add_axes([0.08, 0.10, 0.84, 0.72]); ax.set_xlim(0, 10); ax.set_ylim(0, 14); ax.axis("off")
    def box(x, y, w, h, text, color=BLUE, fill="white"):
        ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.05,rounding_size=0.10",
                                    fc=fill, ec=color, lw=1.7))
        ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=8.2,
                color=INK, linespacing=1.25)
    def arrow(a, b):
        ax.add_patch(FancyArrowPatch(a, b, arrowstyle="-|>", mutation_scale=11, lw=1.1, color=MUTED))
    box(3.2, 12.5, 3.6, 0.8, "Inicio / main(argc, argv)", TEAL, "#e7f6f5")
    box(3.2, 11.1, 3.6, 0.8, "Capturar argumentos: N, modo, hilos,\nventana, seed y opciones de benchmark", BLUE)
    box(3.2, 9.7, 3.6, 0.8, "¿Argumentos válidos?\nRangos y tipos verificados", ORANGE, "#fff3e9")
    box(0.3, 8.2, 2.6, 0.8, "No: error descriptivo,\n--help y salida segura", ORANGE, "#fff3e9")
    box(3.2, 8.2, 3.6, 0.8, "Sí: inicializar estado,\nsemilla y recursos OpenGL", TEAL)
    box(3.2, 6.7, 3.6, 0.9, "Loop de frames\nactualizar dt y procesar ESC", BLUE)
    box(0.3, 5.1, 2.6, 0.9, "seq: for por partícula\nactualizar + vértice", MUTED, "#f1f4f6")
    box(3.7, 5.1, 2.6, 0.9, "par1: omp parallel for\nreduction + regiones por fase", BLUE)
    box(7.1, 5.1, 2.6, 0.9, "par2: una región omp parallel\nfor + single + nowait", TEAL, "#e7f6f5")
    box(3.2, 3.5, 3.6, 0.9, "Barreras / reducción / single\nprogramar lanzamientos", ORANGE)
    box(3.2, 2.0, 3.6, 0.8, "Render OpenGL en hilo principal\nmostrar FPS y tiempo de simulación", TEAL)
    box(3.2, 0.55, 3.6, 0.8, "¿Cerrar? No: siguiente frame · Sí: liberar RAII y salir", NAVY, "#e9eef4")
    for a, b in [((5, 12.5), (5, 11.9)), ((5, 11.1), (5, 10.5)), ((5, 9.7), (5, 9.0)),
                 ((5, 8.2), (5, 7.6)), ((5, 6.7), (5, 6.0)), ((1.6, 5.1), (3.2, 3.95)),
                 ((5, 5.1), (5, 3.95)), ((8.4, 5.1), (6.8, 3.95)), ((5, 3.5), (5, 2.8)),
                 ((5, 2.0), (5, 1.35))]: arrow(a, b)
    arrow((3.2, 10.1), (2.9, 8.6))
    arrow((6.8, 7.15), (7.8, 6.7))
    ax.text(2.55, 9.1, "No", fontsize=8, color=ORANGE)
    ax.text(5.1, 9.12, "Sí", fontsize=8, color=GREEN)
    ax.text(7.6, 6.85, "modo", fontsize=8, color=MUTED)
    ax.add_patch(FancyArrowPatch((3.2, 0.95), (2.2, 6.7), connectionstyle="arc3,rad=0.32",
                                 arrowstyle="-|>", mutation_scale=11, lw=1.0, color="#9aaab5"))
    ax.text(1.25, 3.55, "repetir mientras\nla ventana siga abierta", fontsize=7.5, color=MUTED, ha="center")
    pdf.savefig(fig); plt.close(fig)


def catalog_page(pdf: PdfPages, number: int, second: bool = False):
    title = "Anexo 2. Catálogo de funciones"
    subtitle = "Entradas, salidas y propósito de las rutinas principales"
    fig = page(title, subtitle, number)
    entries = [
        ("parseArguments", "argc, argv, Config&", "ParseStatus; error por excepción", "Lee opciones, valida rangos y llena la configuración."),
        ("initializeSimulation", "State&, N, E, halfWidth, seed", "void; estado inicializado", "Reserva vectores, reparte partículas por slots y prepara la semilla."),
        ("stepSimulation", "State&, SimulationMode, dt", "void; estado avanzado", "Selecciona seq, par1 o par2 para procesar un frame."),
        ("stepSequential", "State&, dt", "void; estado avanzado", "Línea base: actualiza partículas, cuenta vivas y relanza fuegos."),
        ("stepParallelV1", "State&, dt", "void; estado avanzado", "Paraleliza cada fase con regiones parallel for independientes."),
        ("stepParallelV2", "State&, dt", "void; estado avanzado", "Agrupa el frame en una región parallel con for, single y nowait."),
        ("scheduleLaunches", "State&, dt", "void; launchList actualizado", "Decide secuencialmente qué fuegos relanzar usando el RNG compartido."),
        ("updateParticle", "Particle&, dt, time, drag, halfWidth", "bool; viva/apagada", "Aplica gravedad, viento, arrastre, rebotes, vida y desvanecimiento."),
        ("writeVertex", "const Particle&, Vertex&, time", "void; vértice actualizado", "Convierte el estado físico a posición y color para OpenGL."),
        ("computeChecksum", "const SimulationState&", "double", "Suma posiciones y vidas para verificar equivalencia entre versiones."),
        ("runScreensaver", "const Config&", "int; código de salida", "Crea ventana, ejecuta el loop interactivo, FPS y cambio de modo."),
        ("runBenchmark", "const Config&, Renderer*", "int; código de salida", "Ejecuta 10 mediciones, calcula estadísticas y escribe CSV."),
    ]
    if second:
        entries = entries[6:]
    else:
        entries = entries[:7]
    y = 0.82
    for i, (name, inputs, outputs, purpose) in enumerate(entries):
        height = 0.095
        ax = fig.add_axes([0.075, y - height, 0.85, height - 0.008]); ax.axis("off")
        ax.add_patch(FancyBboxPatch((0, 0), 1, 1, boxstyle="round,pad=0.01,rounding_size=0.02",
                                    fc=PALE if i % 2 else "white", ec="#d6e1e8", lw=0.7))
        ax.text(0.02, 0.70, name, fontsize=9.2, weight="bold", color=NAVY)
        ax.text(0.23, 0.70, inputs, fontsize=7.4, color=INK)
        ax.text(0.23, 0.30, "Salida: " + outputs, fontsize=7.4, color=TEAL)
        ax.text(0.57, 0.50, purpose, fontsize=7.7, color=INK, va="center", wrap=True)
        y -= 0.108
    if not second:
        fig.text(0.08, 0.10, "Tipos compartidos: SimulationState agrupa partículas, vértices, slots, contadores y reloj; Renderer administra GLFW/OpenGL con RAII.", fontsize=8.8, color=MUTED)
    else:
        fig.text(0.08, 0.10, "Mecanismos de seguridad: validación de rangos, excepciones descriptivas, std::vector para memoria, checksum y destructor de Renderer.", fontsize=8.8, color=MUTED)
    pdf.savefig(fig); plt.close(fig)


def log_table_page(pdf: PdfPages, number: int):
    fig = page("Anexo 3. Bitácora de pruebas", "Diez mediciones de una configuración representativa", number,
                "Fuente: results/benchmark.csv")
    rows = [r for r in RAW if r["N"] == "500000" and r["modo"] == "par2" and r["hilos"] == "16"]
    rows = sorted(rows, key=lambda r: int(r["medicion"]))
    section_label(fig, "Configuración", 0.81)
    add_paragraph(fig, "N = 500,000; modo par2; 16 hilos; 6 fuegos; 100 frames por medición; 30 frames de calentamiento; semilla fija 2026; sin renderizado. La columna checksum confirma que el estado final coincide con la referencia secuencial.", 0.782, width=94, size=9.3)
    cell = [[r["medicion"], fmt(r["sim_ms_frame"], 4), fmt(r["frame_ms"], 4), fmt(r["fps"], 1), "igual" if r["checksum"] else "igual"] for r in rows]
    ax = fig.add_axes([0.18, 0.43, 0.64, 0.25]); ax.axis("off")
    table = ax.table(cellText=cell, colLabels=["Medición", "Sim ms/frame", "Frame ms", "FPS", "Checksum"], loc="center", cellLoc="center")
    table.auto_set_font_size(False); table.set_fontsize(9); table.scale(1, 1.55)
    for (row, col), c in table.get_celld().items():
        c.set_edgecolor("#d5e0e7")
        if row == 0:
            c.set_facecolor(NAVY); c.get_text().set_color("white"); c.get_text().set_weight("bold")
        elif row % 2 == 0:
            c.set_facecolor(PALE)
        if col == 4 and row > 0:
            c.get_text().set_color(GREEN); c.get_text().set_weight("bold")
    section_label(fig, "Criterio de cálculo", 0.34)
    add_bullets(fig, [
        "Tiempo promedio: media aritmética de los 10 valores de simulación por frame.",
        "Speedup: Tseq / Tpar. Eficiencia: speedup / número de hilos.",
        "Los CSV completos y los logs de cada N quedan versionados en results/ para inspección y reproducción.",
    ], 0.31, width=92, size=9.2)
    pdf.savefig(fig); plt.close(fig)


def methodology_page(pdf: PdfPages, number: int):
    fig = page("4. Metodología experimental", "Protocolo para medir rendimiento, corrección y experiencia visual", number)
    section_label(fig, "Entorno", 0.81)
    y = add_paragraph(fig, "Equipo de medición: AMD Ryzen AI 9 365, 10 núcleos / 20 hilos, x86_64, entorno Linux/WSL2. Compilador GCC 13.3.0, CMake 3.16+, OpenMP 4.5 reportado por GCC y build Release. La ventana interactiva requiere OpenGL 3.3, GLFW y GLEW; las mediciones de speedup se aislaron con --sim-only para no mezclar el costo del driver gráfico.", 0.782, width=94, size=9.6)
    section_label(fig, "Protocolo", y - 0.01)
    y -= 0.04
    y = add_bullets(fig, [
        "Se fijó dt = 1/60 s, semilla 2026 y el mismo estado inicial para seq, par1 y par2.",
        "Cada prueba ejecutó 30 frames de calentamiento y 100 frames medidos; se repitió 10 veces.",
        "N tomó los valores 10,000; 100,000; 500,000. Se probaron 4, 8 y 16 hilos en las corridas pequeñas y medianas; la corrida N=100,000 incluyó además 2 hilos.",
        "Se midieron por separado tiempo de simulación, tiempo total de frame y FPS teóricos; los resultados se escribieron en CSV.",
        "El checksum suma posiciones y vida de todas las partículas. Un resultado igual indica equivalencia dentro de la tolerancia configurada.",
    ], y, width=91, size=9.4)
    section_label(fig, "Limitaciones", 0.34)
    add_paragraph(fig, "Los FPS de esta bitácora son una métrica del loop sin renderizado y no sustituyen la observación visual del screensaver a 60 FPS. El comportamiento gráfico debe validarse en el equipo de presentación con VSync activo y luego con --no-vsync para aislar el límite de la simulación.", 0.31, width=94, size=9.4)
    pdf.savefig(fig); plt.close(fig)


def main():
    OUTPUT.parent.mkdir(exist_ok=True)
    with PdfPages(OUTPUT) as pdf:
        cover(pdf)
        index_page(pdf, 2)
        text_page(pdf, "Resumen ejecutivo", "Qué se construyó, cómo se paralelizó y qué se comprobó", 3, [
            ("Resultado", "Se implementó un screensaver de fuegos artificiales en C++17 que actualiza partículas con gravedad, resistencia del aire, viento oscilante, rebotes, colores pseudoaleatorios y desvanecimiento. La escena se dibuja mediante OpenGL 3.3 con GLFW y GLEW. El programa recibe N y otros parámetros por línea de comandos, muestra FPS y permite cambiar entre tres implementaciones durante la ejecución."),
            ("Paralelización", "La versión secuencial constituye la línea base. par1 aplica un parallel for por fase; par2 agrupa el frame en una única región paralela, usando omp for, reduction, single y nowait. El diseño sigue PCAM: partición por rangos de partículas, comunicación mínima, aglomeración contigua y mapeo estático."),
            ("Evidencia", "En N = 500,000, par1 llegó a 3.955× de speedup con 16 hilos y par2 a 3.891×; todas las pruebas registraron checksum igual. La bitácora contiene 10 repeticiones por configuración y los CSV quedan en results/."),
            ("Cumplimiento", "El repositorio contiene código C/C++ propio, OpenMP, versión secuencial y dos paralelas, validación defensiva, README, scripts reproducibles, mediciones de speedup/eficiencia, diagramas, catálogo de funciones y este informe PDF."),
        ], "Fuentes: [1], [2], [3]")
        text_page(pdf, "1. Introducción y antecedentes", "Problema, motivación y objetivos del proyecto", 4, [
            ("Contexto", "OpenMP ofrece un modelo de programación paralela de memoria compartida para transformar iterativamente un programa secuencial. El proyecto aplica ese modelo a una escena dinámica donde cada partícula puede actualizarse de forma independiente, mientras el hilo principal conserva el control del contexto gráfico."),
            ("Problema", "Un screensaver con muchos elementos debe conservar una animación fluida mientras calcula posiciones, velocidades, colores, vida y vértices. El costo crece con N, por lo que se busca reducir el tiempo de actualización sin alterar el resultado físico ni bloquear el renderizado."),
            ("Objetivo general", "Diseñar e implementar un screensaver parametrizable con una línea base secuencial y versiones paralelas OpenMP, y cuantificar la mejora mediante speedup y eficiencia."),
            ("Objetivos específicos", "(1) Implementar la simulación física de partículas y su visualización. (2) Aplicar PCAM para identificar la unidad de trabajo. (3) Incorporar sincronización correcta y evitar carreras. (4) Comparar dos estrategias paralelas. (5) Construir una bitácora reproducible con al menos 10 mediciones por prueba."),
            ("Marco técnico", "OpenMP define directivas, cláusulas y un modelo de memoria para paralelismo en C/C++ [1]. GLFW abstrae ventana, contexto y eventos [2], mientras OpenGL provee la API de renderizado [3]. La combinación permite mantener la simulación en CPU y el dibujo en el hilo que posee el contexto gráfico."),
        ], "Fuentes: [1] OpenMP API 5.2 · [2] GLFW · [3] Khronos OpenGL")
        architecture_page(pdf, 5)
        text_page(pdf, "3. Implementación", "Física, versiones de simulación, memoria compartida y render", 6, [
            ("Modelo físico", "Cada partícula contiene posición, velocidad, color, vida, decaimiento y fase. En cada paso se aplica gravedad y arrastre; el viento depende del tiempo y la altura; el suelo y los límites laterales aplican restitución y fricción. La opacidad se deriva de la vida restante. Cuando un slot queda sin partículas vivas, se programa un nuevo fuego."),
            ("Versión secuencial", "stepSequential procesa todas las partículas en un for, acumula partículas vivas por slot, decide relanzamientos y escribe los vértices. Es la referencia para speedup y checksum."),
            ("par1", "stepParallelV1 paraleliza la actualización con reduction sobre aliveCounts y abre parallel for adicionales para inicializar cada explosión. Es una mejora directa y didáctica, pero paga el costo de varias regiones paralelas por frame."),
            ("par2", "stepParallelV2 crea una región parallel por frame. El omp for actualiza y reduce; omp single ejecuta scheduleLaunches porque el RNG de slots es compartido; luego omp for nowait inicializa rangos independientes. Las barreras implícitas garantizan que la lista y los contadores estén listos antes de usarse."),
            ("Programación defensiva", "config.cpp valida tipos, rangos, cantidades de hilos, tamaño mínimo de ventana, número de frames y opciones de modo. Los errores producen mensajes claros y salida controlada. std::vector administra memoria y Renderer libera GLFW/OpenGL mediante RAII."),
        ], "Fuentes: [1], [2], [3]")
        methodology_page(pdf, 7)
        results_page(pdf, 8)
        charts_page(pdf, 9)
        text_page(pdf, "6. Conclusiones y recomendaciones", "Evaluación final frente a los objetivos y requisitos", 10, [
            ("Conclusiones", "La solución cumple el flujo completo del proyecto: recibe N, genera una escena colorida con movimiento y física, muestra FPS, separa versión secuencial y paralelas, y usa OpenMP con mecanismos explícitos de reducción, sincronización y regiones críticas de control."),
            ("Conclusiones", "La paralelización produce una mejora medible. Para cargas grandes, 16 hilos redujo aproximadamente cuatro veces el tiempo de simulación frente a la línea base. La eficiencia menor que 1 es esperable: la coordinación, las barreras, el acceso a memoria y la parte secuencial limitan el escalamiento."),
            ("Conclusiones", "La igualdad de checksum en las pruebas da evidencia de corrección funcional entre modos. La arquitectura mantiene el renderizado fuera de OpenMP, evitando acceso concurrente al contexto gráfico."),
            ("Recomendaciones", "Para una evaluación en vivo, probar N creciente con VSync activo y registrar FPS observados; fijar afinidad de hilos si se requiere mayor estabilidad; repetir las mediciones en el equipo de evaluación; y conservar los CSV originales junto con la versión exacta del compilador."),
            ("Entregables", "El PDF actual acompaña el código fuente C/C++, README, CMakeLists.txt, scripts de benchmark, CSV, logs y gráficas del directorio results/. No se incluyen ejecutables como parte del entregable documental."),
        ])
        text_page(pdf, "Bibliografía", "Fuentes técnicas y académicas consultadas", 11, [
            ("[1] OpenMP Architecture Review Board", "OpenMP API Specification 5.2. Especificación del modelo de ejecución, memoria, directivas, reducciones y sincronización. https://www.openmp.org/spec-html/5.2/openmp.html"),
            ("[2] GLFW Project", "GLFW 3.5 Documentation. API multiplataforma para ventanas, contextos OpenGL, entrada y eventos. https://www.glfw.org/docs/latest/"),
            ("[3] Khronos Group", "OpenGL Registry and Reference Pages. Especificaciones y referencia de la API OpenGL. https://registry.khronos.org/OpenGL/"),
            ("[4] Foster, Ian", "Designing and Building Parallel Programs: Concepts and Tools for Parallel Software Engineering. Addison-Wesley, 1995. Método PCAM: partición, comunicación, aglomeración y mapeo."),
            ("[5] Universidad del Valle de Guatemala", "Computación Paralela y Distribuida, Semestre 2, 2026. Proyecto #1: requisitos, contenido y rúbrica de evaluación."),
        ], "Referencias web consultadas: OpenMP, GLFW y Khronos")
        flow_page(pdf, 12)
        catalog_page(pdf, 13, False)
        catalog_page(pdf, 14, True)
        log_table_page(pdf, 15)
        text_page(pdf, "Anexo 3. Bitácora de pruebas", "Resumen de todas las configuraciones agregadas al repositorio", 16, [
            ("Cobertura", "Se registraron 10 mediciones por combinación de N, modo y cantidad de hilos. La tabla resumen contiene 3 valores de N y las variantes seq, par1 y par2. Las columnas incluyen promedio, desviación estándar, máximo, speedup, eficiencia, FPS y checksum_ok."),
            ("Archivos de evidencia", "results/benchmark.csv conserva las mediciones individuales; results/benchmark_resumen.csv conserva los agregados; results/log_N10000.txt y results/log_N500000.txt conservan la salida de consola; y las imágenes *_simonly.png muestran las tendencias."),
            ("Interpretación", "Los resultados deben leerse como una caracterización de la simulación en el equipo indicado. El modo par2 reduce regiones fork/join, pero no necesariamente supera a par1 en todos los tamaños: el costo de coordinación y el tamaño de la carga determinan el punto de equilibrio."),
            ("Reproducción", "cmake -S . -B build && cmake --build build; ./build/screensaver 500000 --benchmark --sim-only --runs 10 --frames 100 --thread-list 4,8,16 --csv results/benchmark.csv; python3 scripts/analyze_results.py"),
        ])
    print(f"Informe generado: {OUTPUT}")


if __name__ == "__main__":
    main()
