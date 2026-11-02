#!/usr/bin/env python3
"""Move the policy interface and the property of benchmark instances into the model.

An instance folder <name>/ holds the model <name>.jani, one .jani2nnet interface per NN, one or more
property files and the files these include. Afterwards the model has a "policy" field with the inputs
and labels of the interfaces, and the property as its only property; the interfaces and property files
are deleted. The policy files (.nnet, fnn/*.json) stay.

Usage: tools/convert_benchmarks.py <instance folder> [<instance folder> ...]
"""

import json
import os
import sys
from pathlib import Path

INTERFACE_KEYS = {"input", "output", "elements", "file", "filter"}


def read(file):
    if not file.exists():
        sys.exit(f"{file}: not found")
    return json.loads(file.read_text(encoding="utf-8"))


def policy(folder, model):
    """Inputs and labels of the interfaces, which must be equal for all of them."""
    automata = [element["automaton"] for element in model["system"]["elements"]]
    fields = []
    for file in sorted(folder.glob("*.jani2nnet")):
        interface = read(file)
        if not set(interface) <= INTERFACE_KEYS:
            sys.exit(f"{file}: unknown keys {sorted(set(interface) - INTERFACE_KEYS)}")
        if interface.get("filter"):
            sys.exit(f"{file}: \"filter\": true is not supported, use --applicability-filtering 1")
        inputs = []
        for feature in interface["input"]:
            if set(feature) != {"name", "automaton"} or "." in feature["name"]:
                sys.exit(f"{file}: input {json.dumps(feature)} is not a variable without \".\" in its name")
            automaton = feature["automaton"]
            if automaton is None:
                inputs.append(feature["name"])
            elif automaton["name"] in automata and automata.index(automaton["name"]) == automaton["index"]:
                inputs.append(f"{automaton['name']}.{feature['name']}")
            else:
                sys.exit(f"{file}: automaton {json.dumps(automaton)} is not the first instance of its name")
        fields.append({"inputs": inputs, "labels": interface["output"]})
    if not fields:
        sys.exit(f"{folder}: no .jani2nnet file")
    if any(f != fields[0] for f in fields):
        sys.exit(f"{folder}: the .jani2nnet files differ in inputs or outputs")
    return fields[0]


def properties(folder, model_file):
    """The property files and the files they include, and the property with the includes inlined."""
    files, found = [], []
    for file in sorted(folder.glob("*.jani")):
        content = read(file)
        if file == model_file or "properties" not in content:
            continue
        if list(content) != ["properties"] or len(content["properties"]) != 1:
            sys.exit(f"{file}: a property file must hold only \"properties\" with one property")
        files.append(file)
        prop = content["properties"][0]
        expression = prop["expression"]
        expression.pop("file", None)
        for key, part in expression.items():
            if isinstance(part, dict) and list(part) == ["file"]:
                files.append(folder / part["file"])
                expression[key] = read(files[-1])
        found.append(prop)
    if not found:
        sys.exit(f"{folder}: no property file")
    if any({**p, "name": ""} != {**found[0], "name": ""} for p in found):
        sys.exit(f"{folder}: the property files differ in more than the name")
    if len(found) > 1:  # one per NN, as compact-property-32_32 and compact-property-64_64
        found[0]["name"] = os.path.commonprefix([p["name"] for p in found]).rstrip("-_")
    return sorted(set(files)), found[0]


def convert(folder):
    model_file = folder / f"{folder.name}.jani"
    if not model_file.exists():
        sys.exit(f"{model_file}: not found")
    text = model_file.read_text(encoding="utf-8")
    model = json.loads(text)
    if "policy" in model:
        sys.exit(f"{model_file}: already converted")
    lines = text.splitlines()
    indent = len(lines[1]) - len(lines[1].lstrip(" ")) if len(lines) > 1 else None
    sort = json.dumps(model, indent=indent, sort_keys=True, ensure_ascii=False) == text.rstrip("\n")

    files, prop = properties(folder, model_file)
    items = [(key, value) for key, value in model.items() if key != "properties"]
    at = list(model).index("properties") if "properties" in model else len(items)
    items[at:at] = [("policy", policy(folder, model)), ("properties", [prop])]
    end = "\n" if text.endswith("\n") else ""
    model_file.write_text(json.dumps(dict(items), indent=indent, sort_keys=sort, ensure_ascii=False) + end, encoding="utf-8")
    for file in files + sorted(folder.glob("*.jani2nnet")):
        file.unlink()


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for folder in sys.argv[1:]:
        convert(Path(folder))


if __name__ == "__main__":
    main()
