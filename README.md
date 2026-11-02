# PlaJA

Repository of the ***PlaJA*** (*Planning in JANI*) code base.

This is a copy of the [original PlaJA code base](https://gitlab.cs.uni-saarland.de/fai/code/plaja),
reduced to what is needed to verify and evaluate FNN and NN policies (fnn2026).

## Input 

Models are to be provided in the automata-based input langauge [JANI](https://jani-spec.org).\
Neural networks (where applicable) are to be provided in [NNet](https://github.com/sisl/nnet) format,
formularized neural networks (FNNs) in the JSON format read by the [FNN parser](policies/fnn).\
The interface between a JANI model and a policy is a top-level field of the model,
```
"policy": {"inputs": ["x", "automaton.y"], "labels": ["a", "b"]}
```
where `inputs` are the variables fed to the policy, in order (`automaton.variable` for a local variable), and
`labels` the actions of the policy outputs, in order.
[tools/convert_benchmarks.py](tools/convert_benchmarks.py) moves the interface and property files of a benchmark instance into its model.
Benchmarks in this format: branch `refactor/structure` of [fnn-benchmarks](https://github.com/vsp-ai/fnn-benchmarks);
the older format on its `master` branch is converted with this script.


## Dependencies

PlaJA uses the [nlohmann JSON library](https://github.com/nlohmann/json) for parsing JANI inputs.\
The [predicate abstraction module](engines/verification) (and other SMT modules) depend on
[Z3](https://github.com/Z3Prover/z3) and 
a [modified version](https://gitlab.cs.uni-saarland.de/vinzent/MarabouPublic.git) of [Marabou](https://github.com/NeuralNetworkVerification/Marabou).\
Marabou depends on [Boost](https://www.boost.org) & [CxxTest](https://github.com/CxxTest/cxxtest),
[OpenBLAS](https://www.openblas.net) & [Gurobi](https://www.gurobi.com).
Gurobi is also used for FNN verification with `--fnn-sat milp`.

PlaJA has been successfully build & run with the following versions of the dependencies listed above (on Linux and MacOS):
- nlohmann JSON library version [3.11.2](https://github.com/nlohmann/json/tree/v3.11.2)
- Z3 version [4.8.12](https://github.com/Z3Prover/z3/tree/z3-4.8.12) 
commit [3a402ca2c14c3891d24658318406f80ce59b719f](https://github.com/Z3Prover/z3/tree/3a402ca2c14c3891d24658318406f80ce59b719f)
(July 13, 2021)
- Boost version [1.81.0](https://sourceforge.net/projects/boost/files/boost/1.81.0/boost_1_81_0.tar.gz) on Linux and on MacOS.
- OpenBlas version [0.3.19](https://github.com/xianyi/OpenBLAS/releases/download/v0.3.19/OpenBLAS-0.3.19.tar.gz).
- Gurobi version [12.0.0](https://www.gurobi.com/downloads/gurobi-optimizer-eula/).
- CxxTest version [4.4](https://github.com/CxxTest/cxxtest/tree/4.4) commit [191adddb3876ab389c0c856e1c03874bf70f8ee4](https://github.com/CxxTest/cxxtest/tree/191adddb3876ab389c0c856e1c03874bf70f8ee4) (June 4, 2014).

## Building

The code base is built using [CMake](https://cmake.org).
See [CMakeLists.txt](CMakeLists.txt) and [DependenciesConfig.cmake](DependenciesConfig.cmake) for configuration.
A typical build is
```
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make PlaJA
```
The [Dockerfile](Dockerfile) provides an image with the dependencies (except Gurobi).

The cmake infrastructure has been successfully tested on Linux and MacOS.


## Running

After building the ***PlaJA*** executable can be found in the respective source directory.

To see basic commandline options you can run
```
./PlaJA --help
```
If you **additionally** set the engine option then commandline options specific to that engine will be provided.
For instance,
```
./PlaJA --help --engine PA_CEGAR
```
will additionally output the commandline options specific to the PA_CEGAR engine.

PlaJA provides two engines: `PA_CEGAR` verifies the safety of a policy (NN or FNN) by predicate abstraction
with counterexample-guided refinement, and `POLICY_EVALUATION` simulates a policy (NN, FNN or a built-in expert policy).
For instance,
```
./PlaJA --model-file model.jani --engine PA_CEGAR --fnn policy.json
```
runs PA_CEGAR for each property of the model, and `--prop 1` picks the property with index 1.

`--stats-file <file>` appends the main statistics of each run to a CSV file; with `--stats-verbose` it writes all statistics.


## Layout

- [core](core): search engine base and factory, configuration, states, options, statistics, timer, exceptions and utilities.
- [model](model): JANI parser, AST, visitors, and model, property and synchronisation information.
- [successor_generation](successor_generation): explicit successor generation.
- [policies](policies): policy interface, NN, FNN and expert policies; no solver needed.
- [smt](smt): Z3 layer, with encodings of the model and FNNs; [smt/base](smt/base) and the base solution enumerators are solver-agnostic, Marabou builds on the former.
- [marabou](marabou): Marabou layer.
- [pseudo_boolean](pseudo_boolean): pseudo-Boolean formulas and the RoundingSat solver.
- [milp](milp): Z3 queries translated to Gurobi.
- [engines](engines): start-state enumeration, policy evaluation (`POLICY_EVALUATION`) and predicate abstraction (`PA_CEGAR`) with its NN and FNN checkers.
- [include](include): compile-time configuration and Marabou headers.
- [tests](tests): regression suite and PB unit tests.
- [tools](tools): conversion of benchmark instances to the format above.


## References & Credits

PlaJA implements a basic search engine infrastructure adapting an [extract](core/REFERENCES.md) of a [stripped Fast Downward code base](https://gitlab.cs.uni-saarland.de/fai/teaching/FD-Stripped.git) (non-public).

The [successor generation](successor_generation) has been designed under the impression of [PRISM](https://github.com/prismmodelchecker/prism).

PlaJA contains a reimplementation of Marabou's [neural network parsing functionality](policies/nn/neural_network.cpp).\
Moreover, for customization purposes, PlaJA also adapts Marabou's [input query generation](marabou/nn_in_marabou.cpp).

Some [auxiliary classes](core/utils/fd_imports) are plain [Fast Downward](https://github.com/aibasel/downward) imports,
while other adapt from similar routines [Fast Downward](core/utils/priority_queue.h) and [Marabou](core/utils/floating_utils.h).

For further notes see the reference files in the respective directories.


## LICENSE

PlaJA is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

PlaJA is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.

