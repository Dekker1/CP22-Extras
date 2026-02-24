#!/usr/bin/env python3
from __future__ import annotations

import csv
import math
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Dict, Iterable, List, Optional

from jobshop.extradata import fndbnd, mkspan


@dataclass(frozen=True)
class BoundRule:
    extra_name: str
    bnd_fn: Callable[[int, int], int]


OBJECTIVES = ("MinDis", "MaxEff")

MIN_DIS_RULE = BoundRule(
    extra_name="MinDis",
    bnd_fn=lambda fnd, _n_agents: math.ceil(fnd * 0.8),
)

MAX_EFF_BY_MODEL: Dict[str, BoundRule] = {
    "jobshop/jobshop.mzn": BoundRule(
        extra_name="MaxEffSpread",
        bnd_fn=lambda fnd, n_agents: math.floor(((fnd / n_agents) * 0.2) ** 2),
    ),
    "jobshop/jobshop_gini.mzn": BoundRule(
        extra_name="MaxEffGini",
        bnd_fn=lambda _fnd, _n_agents: 2000,
    ),
}

DZN_AGENTS_RE = re.compile(r"_(\d+)a_")
N_AGENTS_RE = re.compile(r"\bn_agents\s*=\s*(\d+)\s*;")


def read_n_agents(source_path: Path, repo_root: Path, cache: Dict[Path, int]) -> int:
    name_match = DZN_AGENTS_RE.search(source_path.name)
    if name_match is not None:
        return int(name_match.group(1))

    resolved = source_path if source_path.is_absolute() else (repo_root / source_path)

    if resolved in cache:
        return cache[resolved]

    match = N_AGENTS_RE.search(resolved.read_text(encoding="utf-8"))
    if match is None:
        raise ValueError(f"Could not determine n_agents for {resolved}")

    n_agents = int(match.group(1))
    cache[resolved] = n_agents
    return n_agents


def rule_for_row(row: Dict[str, str], objective: str) -> Optional[BoundRule]:
    if objective == "MinDis":
        return MIN_DIS_RULE

    if objective == "MaxEff":
        return MAX_EFF_BY_MODEL.get(row["model"])

    raise ValueError(f"Unsupported objective: {objective}")


def filter_rows(
    rows: Iterable[Dict[str, str]],
    objective: str,
    generated_root: Path,
    repo_root: Path,
) -> List[Dict[str, str]]:
    filtered: List[Dict[str, str]] = []
    n_agents_cache: Dict[Path, int] = {}

    for row in rows:
        rule = rule_for_row(row, objective)
        if rule is None:
            continue

        data_file = row["data_file"].split(":", maxsplit=1)[0]
        source_path = Path(data_file)
        name = source_path.name

        if name not in mkspan or name not in fndbnd:
            continue

        n_agents = read_n_agents(source_path, repo_root, n_agents_cache)
        end = math.floor(mkspan[name] * 0.5)
        bnd = rule.bnd_fn(fndbnd[name], n_agents)

        extra_dir = generated_root / rule.extra_name
        extra_dir.mkdir(parents=True, exist_ok=True)

        extra_path = extra_dir / f"{source_path.stem}_{rule.extra_name}.dzn"
        extra_path.write_text(f"end = {end};\nbnd = {bnd};\n", encoding="utf-8")

        extra_path_rel = extra_path.relative_to(repo_root).as_posix()
        updated = dict(row)
        updated["data_file"] = f"{data_file}:{extra_path_rel}"
        filtered.append(updated)

    return filtered


def main() -> None:
    repo_root = Path(__file__).resolve().parent
    instances_csv = repo_root / "jobshop_instances.csv"

    with instances_csv.open(newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        if reader.fieldnames != ["problem", "model", "data_file"]:
            raise ValueError("jobshop_instances.csv must have header: problem,model,data_file")
        rows = list(reader)

    generated_root = repo_root / "jobshop" / "generated_bounds"

    for objective in OBJECTIVES:
        selected_rows = filter_rows(rows, objective, generated_root, repo_root)

        output_csv = repo_root / f"jobshop_instances_{objective}.csv"
        with output_csv.open("w", newline="", encoding="utf-8") as fh:
            writer = csv.DictWriter(fh, fieldnames=["problem", "model", "data_file"], dialect="unix")
            writer.writeheader()
            writer.writerows(selected_rows)

        print(f"{objective}: wrote {len(selected_rows)} instances to {output_csv}")


if __name__ == "__main__":
    main()
