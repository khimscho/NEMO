#!/usr/bin/env bash

/tmp/cmake/bin/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B build-$(uname -m) -S . \
  -DN2K_INCLUDE=${N2K_INCLUDE} -DN2K_LIB=${N2K_LIB}
/tmp/cmake/bin/cmake --build build-$(uname -m) -j $(nproc)
