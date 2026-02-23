import sys
from datetime import timedelta
from enum import Enum
from pathlib import Path

import minizinc
from mzn_bench import Configuration, schedule


class Objective(str, Enum):
    MAX_EFF = "MaxEff"
    MIN_DIS = "MinDis"


if len(sys.argv) != 2:
    raise SystemExit(f"usage: {Path(sys.argv[0]).name} <MaxEff|MinDis>")

try:
    OBJECTIVE = Objective(sys.argv[1])
except ValueError as exc:
    raise SystemExit("error: objective must be one of: MaxEff, MinDis") from exc

CONFIGURATIONS = [
    Configuration(
        "Gecode Dcmp",
        solver=minizinc.Solver.lookup("org.gecode.gecode"),
        extra_data={"mode": -2},
    ),
    Configuration(
        "Chuffed Dcmp",
        solver=minizinc.Solver.lookup("org.chuffed.chuffed"),
        extra_data={"mode": -2},
    ),
    Configuration(
        "Gecode BB",
        solver=minizinc.Solver.lookup("org.gecode.gecode"),
        extra_data={"mode": 666},
    ),
    Configuration(
        "Chuffed BB",
        solver=minizinc.Solver.lookup("org.chuffed.chuffed"),
        extra_data={"mode": 666},
    ),
    Configuration(
        "Chuffed Prop",
        solver=minizinc.Solver.lookup("org.chuffed.chuffed_aekh"),
        extra_data={"mode": 666},
    ),
]

for c in range(len(CONFIGURATIONS)):
    CONFIGURATIONS[c].extra_data["Case"] = f"{OBJECTIVE.value}"

schedule(
    instances=Path("./jobshop_instances.csv"),
    timeout=timedelta(minutes=20),
    configurations=CONFIGURATIONS,
    memory=8192,
    nodelist=["critical001"],
    output_dir=Path(f"./output/{OBJECTIVE.value}/"),
)
