#!/usr/bin/env python3
from __future__ import annotations

import argparse
import math
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
from bokeh import palettes as bokeh_palettes
from matplotlib import ticker
from mzn_bench.analysis.plot import plot_primal_integral

RENAME_MAP = {
    "Chuffed Dcmp": "Decomposition",
    "Chuffed BBval": "BB (Value)",
    "Chuffed BB": "BB (Bounds)",
    "Chuffed Prop": "Propagator",
}


def resolve_palette(name: str) -> list[str]:
    palette = getattr(bokeh_palettes, name, None)
    if isinstance(palette, dict) and palette:
        palette = palette[max(palette)]
    if not isinstance(palette, (list, tuple)) or len(palette) == 0:
        raise ValueError(
            f"Unknown palette '{name}'. Use a Bokeh palette name such as "
            "Category10_5, Spectral5, Viridis8, Turbo256, or Spectral."
        )
    return list(palette)


def subplot_title(solns_csv: Path) -> str:
    name = solns_csv.stem.lower()
    metric = "Spread" if "spread" in name else "Gini" if "gini" in name else "Other"
    objective = (
        "MinDis" if "mindis" in name else "MaxEff" if "maxeff" in name else "Other"
    )
    return f"{metric}, {objective}"


def axis_group(solns_csv: Path) -> str | None:
    name = solns_csv.stem.lower()
    if "maxeff" in name:
        return "maxeff"
    return None


def axis_kind(solns_csv: Path) -> str:
    name = solns_csv.stem.lower()
    if "mindis" in name:
        return "mindis"
    if "maxeff" in name:
        return "maxeff"
    return "other"


def subplot_order_key(solns_csv: Path) -> tuple[int, int, str]:
    name = solns_csv.stem.lower()
    row = 0 if "mindis" in name else 1 if "maxeff" in name else 2
    col = 0 if "spread" in name else 1 if "gini" in name else 2
    return (row, col, name)


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Create one combined image for all solns*.csv primal-integral plots "
            "with a single shared legend."
        )
    )
    parser.add_argument(
        "--results-dir",
        default="results",
        help="Directory containing solns*.csv files (default: results)",
    )
    parser.add_argument(
        "--out",
        default="results/primal_all.png",
        help="Output image path (default: results/primal_all.png)",
    )
    parser.add_argument("--palette", default="Category10_5")
    parser.add_argument("--par", type=int, default=2)
    parser.add_argument("--log-y", action="store_true")
    parser.add_argument(
        "--title-fontsize",
        type=float,
        default=16,
        help="Font size for subplot titles (default: 16)",
    )
    parser.add_argument(
        "--label-fontsize",
        type=float,
        default=14,
        help="Font size for axis labels (default: 14)",
    )
    parser.add_argument(
        "--tick-fontsize",
        type=float,
        default=12,
        help="Font size for axis tick labels (default: 12)",
    )
    parser.add_argument(
        "--legend-fontsize",
        type=float,
        default=13,
        help="Font size for legend text (default: 13)",
    )
    args = parser.parse_args()

    results_dir = Path(args.results_dir)
    solns_files = sorted(results_dir.glob("solns*.csv"), key=subplot_order_key)
    if not solns_files:
        raise SystemExit(f"No solns CSV files found in {results_dir}")

    palette = resolve_palette(args.palette)

    n = len(solns_files)
    ncols = 2
    nrows = math.ceil(n / ncols)
    fig, axes = plt.subplots(
        nrows=nrows,
        ncols=ncols,
        figsize=(14, 4.5 * nrows),
        squeeze=False,
    )

    label_to_handle: dict[str, object] = {}
    group_axes: dict[str, list[plt.Axes]] = {"maxeff": []}
    kind_by_axis: dict[plt.Axes, str] = {}

    for i, solns_csv in enumerate(solns_files):
        ax = axes[i // ncols][i % ncols]
        solns = pd.read_csv(solns_csv)
        tmp_fig = plot_primal_integral(
            solns,
            par=args.par,
            palette=palette,
            title=subplot_title(solns_csv),
            x_label="Time (s)",
            y_label="Objective",
            log_y=args.log_y,
            show_legend=False,
            configuration_order=list(RENAME_MAP.keys()),
            rename_configurations=RENAME_MAP,
        )

        src_ax = tmp_fig.axes[0]
        for line in src_ax.lines:
            (new_line,) = ax.step(
                line.get_xdata(),
                line.get_ydata(),
                where="post",
                color=line.get_color(),
                label=line.get_label(),
            )
            if line.get_label() and line.get_label() not in label_to_handle:
                label_to_handle[line.get_label()] = new_line

        ax.set_title(subplot_title(solns_csv), fontsize=args.title_fontsize)
        ax.set_xlabel("Time (s)", fontsize=args.label_fontsize)
        ax.set_ylabel("Objective", fontsize=args.label_fontsize)
        ax.tick_params(axis="both", labelsize=args.tick_fontsize)
        if args.log_y:
            ax.set_yscale("log")

        kind_by_axis[ax] = axis_kind(solns_csv)
        group = axis_group(solns_csv)
        if group is not None:
            group_axes[group].append(ax)

        plt.close(tmp_fig)

    for axes_in_group in group_axes.values():
        if len(axes_in_group) < 2:
            continue
        y_mins: list[float] = []
        y_maxs: list[float] = []
        for ax in axes_in_group:
            for line in ax.lines:
                y = line.get_ydata()
                if len(y) == 0:
                    continue
                y_mins.append(float(y.min()))
                y_maxs.append(float(y.max()))
        if not y_mins or not y_maxs:
            continue
        ymin = min(y_mins)
        ymax = max(y_maxs)
        span = ymax - ymin
        pad = span * 0.04 if span > 0 else max(1.0, abs(ymax) * 0.04)
        ymin_padded = ymin - pad
        ymax_padded = ymax + pad
        for ax in axes_in_group:
            ax.set_ylim(ymin_padded, ymax_padded)

    if not args.log_y:
        for ax, kind in kind_by_axis.items():
            if kind == "mindis":
                ax.ticklabel_format(axis="y", style="sci", scilimits=(0, 0))
            elif kind == "maxeff":
                ax.ticklabel_format(axis="y", style="sci", scilimits=(0, 0))
                ax.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))
            ax.yaxis.get_offset_text().set_fontsize(args.tick_fontsize)

    total_axes = nrows * ncols
    for i in range(n, total_axes):
        axes[i // ncols][i % ncols].set_visible(False)

    if label_to_handle:
        legend = fig.legend(
            label_to_handle.values(),
            label_to_handle.keys(),
            loc="upper center",
            ncol=min(4, len(label_to_handle)),
            bbox_to_anchor=(0.5, 1.0),
            fontsize=args.legend_fontsize,
        )
        legend.set_in_layout(True)

    fig.tight_layout(rect=(0, 0, 1, 0.94))
    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=180)
    print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()
