#!/usr/bin/env python3
"""Split one result CSV by model into spread and gini files.

Writes next to the input file:
  <stem>_spread.csv (rows where model does NOT contain "gini")
  <stem>_gini.csv   (rows where model contains "gini")

Example:
  python split_results_by_model.py results/MaxEff.csv
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


def classify_model(model_value: str) -> str:
    """Return split label for a model value."""
    normalized = (model_value or "").strip().lower()
    return "gini" if "gini" in normalized else "spread"


def split_file(path: Path) -> tuple[Path, int, Path, int]:
    with path.open("r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        if not reader.fieldnames:
            raise ValueError(f"No header found in {path}")
        if "model" not in reader.fieldnames:
            raise ValueError(f"Missing 'model' column in {path}")

        base = path.with_suffix("")
        spread_path = base.parent / f"{base.name}_spread.csv"
        gini_path = base.parent / f"{base.name}_gini.csv"

        spread_rows: list[dict[str, str]] = []
        gini_rows: list[dict[str, str]] = []

        for row in reader:
            group = classify_model(row.get("model", ""))
            if group == "gini":
                gini_rows.append(row)
            else:
                spread_rows.append(row)

    for out_path, rows in ((spread_path, spread_rows), (gini_path, gini_rows)):
        with out_path.open("w", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=reader.fieldnames)
            writer.writeheader()
            writer.writerows(rows)

    return spread_path, len(spread_rows), gini_path, len(gini_rows)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "csv_file",
        type=Path,
        help="Input CSV file to split",
    )
    args = parser.parse_args()

    csv_path = args.csv_file
    if not csv_path.exists() or not csv_path.is_file():
        raise SystemExit(f"Not a file: {csv_path}")
    if csv_path.suffix.lower() != ".csv":
        raise SystemExit(f"Expected a .csv file: {csv_path}")

    spread_path, spread_count, gini_path, gini_count = split_file(csv_path)
    print(
        f"{csv_path.name} -> {spread_path.name} ({spread_count}), {gini_path.name} ({gini_count})"
    )
