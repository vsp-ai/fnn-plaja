# PlaJA

Repository of the ***PlaJA*** (*Planning in JANI*) code base.

This is a copy of the [original PlaJA code base](https://gitlab.cs.uni-saarland.de/fai/code/plaja),
reduced to what is needed to verify and evaluate FNN and NN policies (fnn2026).

## Input 

Models are to be provided in the automata-based input langauge [JANI](https://jani-spec.org).\
Neural networks (where applicable) are to be provided in [NNet](https://github.com/sisl/nnet) format,
formularized neural networks (FNNs) in the JSON format read by the [FNN parser](parser/fnn_parser).\
The interface between a JANI model and a policy is to be documented in [Jani2NNet](search/information/jani2nnet/jani2nnet_interface.jani2nnet) format.


## Dependencies

PlaJA uses the [nlohmann JSON library](https://github.com/nlohmann/json) for parsing JANI inputs.\
The [predicate abstraction module](search/predicate_abstraction) (and other SMT modules) depend on
[Z3](https://github.com/Z3Prover/z3) and 
a [modified version](https://gitlab.cs.uni-saarland.de/vinzent/MarabouPublic.git) of [Marabou](https://github.com/NeuralNetworkVerification/Marabou).\
Marabou depends on [Boost](https://www.boost.org) & [CxxTest](https://github.com/CxxTest/cxxtest)
and (optionally) [OpenBLAS](https://www.openblas.net) & [Gurobi](https://www.gurobi.com).
(Using both is the only configuration actively maintained though.)
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
A property is picked by its index, e.g.,
```
./PlaJA --model-file model.jani --additional-properties additional_properties.jani --prop 1 --engine PA_CEGAR --fnn-interface policy.jani2nnet --fnn policy.json
```
runs PA_CEGAR for the property with index 1, where the properties of the *additional-properties-file* extend
the properties array of the model file.
The additional properties file must contain a single JSON object composed of a JANI properties array.
PlaJA will treat this array as an extension of the properties array in the model file (if present).


## References & Credits

PlaJA implements a basic search engine infrastructure adapting an [extract](search/fd_adaptions) of a [stripped Fast Downward code base](https://gitlab.cs.uni-saarland.de/fai/teaching/FD-Stripped.git) (non-public).

The [successor generation](search/successor_generation) has been designed under the impression of [PRISM](https://github.com/prismmodelchecker/prism).

PlaJA contains a reimplementation of Marabou's [neural network parsing functionality](parser/nn_parser).\
Moreover, for customization purposes, PlaJA also adapts Marabou's [input query generation](search/smt_nn/nn_in_marabou.cpp).

Some [auxiliary classes](utils/fd_imports) are plain [Fast Downward](https://github.com/aibasel/downward) imports,
while other adapt from similar routines [Fast Downward](utils/priority_queue.h) and [Marabou](utils/floating_utils.h).

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

