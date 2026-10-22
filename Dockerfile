# Dependency image for building and running PlaJA (FNN/NN policy evaluation and verification).
# It holds the toolchain and prebuilt dependencies only; PlaJA is compiled from this source tree.
# Gurobi is not included: mount it at /workspace/dependencies/gurobi (see scripts/setup.sh).
#
#   docker build -t fnn2026-deps:v1 - < Dockerfile

FROM victorsputrich/plaja_dependencies-chaahat:MRv0.5.1-roundingsat-ipasir AS prebuilt

FROM ubuntu:22.04 AS base

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=Etc/UTC
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ca-certificates wget libgomp1 \
    && rm -rf /var/lib/apt/lists/*

ARG D=/workspace/dependencies
COPY --from=prebuilt ${D}/z3/src/api ${D}/z3/src/api
COPY --from=prebuilt ${D}/z3/build/libz3.so* ${D}/z3/build/
COPY --from=prebuilt ${D}/json/single_include ${D}/json/single_include
COPY --from=prebuilt ${D}/cxxtest ${D}/cxxtest
COPY --from=prebuilt ${D}/Marabou/src ${D}/Marabou/src
COPY --from=prebuilt ${D}/Marabou/c++/api ${D}/Marabou/c++/api
COPY --from=prebuilt ${D}/Marabou/deps/CVC4 ${D}/Marabou/deps/CVC4
COPY --from=prebuilt ${D}/Marabou/build/libMarabouHelper.a ${D}/Marabou/build/
COPY --from=prebuilt ${D}/Marabou/tools/boost_1_81_0/installed ${D}/Marabou/tools/boost_1_81_0/installed
COPY --from=prebuilt ${D}/Marabou/tools/OpenBLAS-0.3.19/installed ${D}/Marabou/tools/OpenBLAS-0.3.19/installed

ENV GUROBI_HOME=${D}/gurobi/linux \
    LD_LIBRARY_PATH=${D}/gurobi/linux/lib
LABEL org.opencontainers.image.title="fnn2026-deps" \
      z3="4.8.12" marabou="48cfdd9" boost="1.81.0" openblas="0.3.19" gurobi.expected="12.0.0"
WORKDIR /workspace

# Internal only (RoundingSat-ipasir is not redistributable yet): --target roundingsat
FROM base AS roundingsat
ARG D=/workspace/dependencies
COPY --from=prebuilt ${D}/roundingsat-ipasir/src ${D}/roundingsat-ipasir/src
COPY --from=prebuilt ${D}/roundingsat-ipasir/build-docker-ipasir/librsipasir.so ${D}/roundingsat-ipasir/build-docker-ipasir/

FROM base AS deps
