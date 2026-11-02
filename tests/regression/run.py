#!/usr/bin/env python3
"""Run the cases in cases.csv with a PlaJA binary and compare the statistics with golden.csv.

Examples:
  tests/regression/run.py --plaja build/PlaJA --bench ../benchmarks
  tests/regression/run.py --plaja build/PlaJA --bench ../benchmarks --quick

Instances hold the policy interface and the property in the model (tools/convert_benchmarks.py converts the
older format). golden.csv holds the results of the fnn2026 reference binary on the older format.
All runs share one container of the PlaJA image. Gurobi and RoundingSat are taken from
<bench>/../.cache/gurobi and <bench>/../.cache/roundingsat.
MILP and Marabou-with-Gurobi cases need a license (GRB_LICENSE_FILE, default ~/gurobi.lic), PB cases a
binary built with RoundingSat; otherwise they are skipped.
"""

import argparse
import csv
import hashlib
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

HERE = Path(__file__).resolve().parent
IMAGE = os.environ.get("PLAJA_IMAGE", "faisaarlab/fnn2026-deps:v1")
GUROBI_LICENSE = Path(os.environ.get("GRB_LICENSE_FILE", Path.home() / "gurobi.lic"))
JOBS = 4  # cases run in parallel; a full run peaks at about 220 MB in the container
TIMEOUT = 3600  # seconds per case; the longest (z3-unsafe) takes about 20 minutes

EVALUATE = ["--engine", "POLICY_EVALUATION", "--initial-state-enum", "sample",
            "--applicability-filtering", "1", "--num-episodes", "3000", "--seed", "42"]

VERIFY = ["--engine", "PA_CEGAR", "--applicability-filtering", "1", "--heuristic-search", "hamming",
          "--check-for-pa-terminal-states", "true", "--set-pa-goal-objective-terminal", "false",
          "--pa-state-aware", "true", "--split-point", "bs", "--selection-refinement", "all",
          "--entailment-mode", "entailment-only", "--check-policy-spuriousness", "false",
          "--incremental-search", "true", "--seed", "42"]

ENGINES = {
    "eval": EVALUATE,
    "z3": VERIFY + ["--fnn-sat", "z3"],
    "milp": VERIFY + ["--fnn-sat", "milp"],
    "pb-flat": VERIFY + ["--fnn-sat", "pb-constraint", "--use-incremental-pb", "false"],
    "pb-inc": VERIFY + ["--fnn-sat", "pb-constraint", "--use-incremental-pb", "true",
                        "--incremental-pb-granularity", "label"],
    "marabou": VERIFY,
    "marabou-native": VERIFY + ["--marabou-solver", "reluplex", "--marabou-options", "--lp-solver native"],
}

# Not compared: times, memory, and what only names the inputs.
IGNORED = {"property", "commandline", "memory_peak"}

# Statistics that differ between two runs of the same binary, with the reason. Two runs of the
# fnn2026-v1 reference over all cases, Gurobi cases included, were identical in every statistic
# and trace.
UNSTABLE = {
}


def command(bench, case):
    """PlaJA arguments of a case; model and policy are found in the instance folder."""
    instance, policy = case["instance"], case["policy"]

    def path(file):
        if not (bench / instance / file).exists():
            raise SystemExit(f"Not found: {bench / instance / file}")
        return f"/bench/{instance}/{file}"

    arguments = ENGINES[case["engine"]] + ["--model-file", path(f"{instance}.jani")]
    if policy.endswith(".nnet"):
        arguments += ["--nn", path(policy)]
    elif policy.endswith(".json"):
        arguments += ["--fnn", path(policy)]
    else:
        arguments += ["--expert-policy", policy]
    return arguments + shlex.split(case["options"])


def read_stats(path):
    with open(path, newline="") as f:
        rows = list(csv.reader(f))
    stats = {}
    for name, value in zip(rows[0], rows[-1]):
        key, n = name, 1
        while key in stats:  # PA_CEGAR writes some columns twice: overall, then for the last search
            n += 1
            key = f"{name}#{n}"
        stats[key] = value
    return stats


def compared(name):
    name = name.split("#")[0]
    return "Time" not in name and name not in IGNORED and name not in UNSTABLE


def results(folder, exit_code):
    stats = {"exit": str(exit_code)}
    if (folder / "stats.csv").exists():
        stats.update((k, v) for k, v in read_stats(folder / "stats.csv").items() if compared(k))
    for file in sorted(folder.iterdir()):
        if file.name not in ("stats.csv", "log"):
            stats[f"sha256 {file.name}"] = hashlib.sha256(file.read_bytes()).hexdigest()
    return stats


def run_case(container, plaja, bench, out, case):
    folder = out / case["name"]
    folder.mkdir()
    arguments = command(bench, case) + ["--stats-file", "stats.csv", "--stats-verbose"]
    start = time.time()
    with open(folder / "log", "w") as log:
        exit_code = subprocess.run(["docker", "exec", "-w", f"/out/{case['name']}", container,
                                    "timeout", "-k", "60", str(TIMEOUT), f"/plaja/{plaja.name}"] + arguments,
                                   stdout=log, stderr=subprocess.STDOUT).returncode
    return results(folder, exit_code), time.time() - start


def log_tail(folder):
    lines = (folder / "log").read_text(errors="replace").splitlines()
    return " | ".join(lines[-2:])


def start_container(name, plaja, bench, out, gurobi, roundingsat):
    docker = ["docker", "run", "-d", "--rm", "--name", name, "--memory", "12g", "--memory-swap", "12g",
              "-u", f"{os.getuid()}:{os.getgid()}",
              "-v", f"{plaja.parent}:/plaja:ro", "-v", f"{bench}:/bench:ro", "-v", f"{out}:/out",
              "-v", f"{gurobi}:/workspace/dependencies/gurobi:ro"]
    if GUROBI_LICENSE.is_file():
        docker += ["-v", f"{GUROBI_LICENSE}:/gurobi.lic:ro", "-e", "GRB_LICENSE_FILE=/gurobi.lic"]
    if roundingsat:
        docker += ["-v", f"{roundingsat}:/workspace/dependencies/roundingsat:ro"]
    subprocess.run(docker + [IMAGE, "sleep", "infinity"], check=True, stdout=subprocess.DEVNULL)


def read_golden(update):
    golden = {}
    if not (HERE / "golden.csv").exists():
        if update:
            return golden
        raise SystemExit(f"{HERE / 'golden.csv'} not found; make it with --update")
    with open(HERE / "golden.csv", newline="") as f:
        for row in csv.DictReader(f):
            golden.setdefault(row["case"], {})[row["statistic"]] = row["value"]
    return golden


def write_golden(golden, cases):
    with open(HERE / "golden.csv", "w", newline="") as f:
        writer = csv.writer(f, lineterminator="\n")
        writer.writerow(["case", "statistic", "value"])
        for case in cases:
            for name, value in golden.get(case["name"], {}).items():
                writer.writerow([case["name"], name, value])


def differences(expected, actual):
    names = [n for n in expected if compared(n)] + [n for n in actual if n not in expected]
    return [f"{n} {expected.get(n, '-')} != {actual.get(n, '-')}" for n in names if expected.get(n) != actual.get(n)]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--plaja", required=True, type=Path, help="PlaJA binary")
    parser.add_argument("--bench", required=True, type=Path, help="benchmarks folder")
    parser.add_argument("--update", action="store_true", help="write the results to golden.csv")
    parser.add_argument("--quick", action="store_true", help="only the cases marked quick")
    args = parser.parse_args()
    sys.stdout.reconfigure(line_buffering=True)
    plaja = args.plaja.resolve()
    bench = args.bench.resolve()
    cache = bench.parent / ".cache"
    gurobi = cache / "gurobi"
    roundingsat = None
    if b"librsipasir.so" in plaja.read_bytes():  # linked against RoundingSat-ipasir, i.e. built with PB
        roundingsat = cache / "roundingsat"

    with open(HERE / "cases.csv", newline="") as f:
        cases = list(csv.DictReader(f))
    selected = [c for c in cases if c["quick"] == "yes" or not args.quick]
    golden = read_golden(args.update)
    skip = {}
    if not GUROBI_LICENSE.is_file():
        skip |= dict.fromkeys(["milp", "marabou"], f"no Gurobi license ({GUROBI_LICENSE})")
    if roundingsat is None:
        skip |= dict.fromkeys(["pb-flat", "pb-inc"], "PlaJA built without RoundingSat")

    out = Path(tempfile.mkdtemp(prefix="plaja-regression-"))
    container = f"plaja-regression-{os.getpid()}"
    pool = ThreadPoolExecutor(max_workers=JOBS)
    failed = False
    try:
        start_container(container, plaja, bench, out, gurobi, roundingsat)
        jobs = {}
        for case in selected:  # cases.csv lists the longest cases first
            if case["engine"] in skip:
                print(f"{case['name']:40} skip ({skip[case['engine']]})")
                continue
            jobs[pool.submit(run_case, container, plaja, bench, out, case)] = case
        for job in as_completed(jobs):
            case = jobs[job]
            actual, seconds = job.result()
            expected = golden.get(case["name"], {})
            if args.update:
                golden[case["name"]] = actual
                status = "updated" if "Status" in actual and actual["exit"] == "0" else "failed"
            elif not expected:
                status = "not in golden.csv"
            elif actual["exit"] != expected["exit"]:
                status = f"exit {actual['exit']} instead of {expected['exit']}"
            else:
                diff = differences(expected, actual)
                status = "ok" if not diff else "; ".join(diff)
            if status not in ("ok", "updated"):
                failed = True
                if actual["exit"] != "0":
                    status += f": {log_tail(out / case['name'])}"
            print(f"{case['name']:40} {status}  ({seconds:.0f} s)")
    except BaseException:
        print(f"Logs: {out}")
        raise
    finally:
        pool.shutdown(wait=False, cancel_futures=True)
        subprocess.run(["docker", "rm", "-f", container], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    if failed:
        print(f"Logs: {out}")
        sys.exit(1)
    shutil.rmtree(out)
    if args.update:
        write_golden(golden, cases)


if __name__ == "__main__":
    main()
