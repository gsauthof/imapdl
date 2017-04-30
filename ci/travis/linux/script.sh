#!/bin/bash

set -ex

docker exec devel env \
  CFLAGS="$CFLAGS" \
  CXXFLAGS="$CXXFLAGS" \
  LDFLAGS="$LDFLAGS" \
  build_method="$build_method" \
  build_type="$build_type" \
  build_flags="$build_flags" \
  targets="$targets" \
  UT_PREFIX="$UT_PREFIX" \
  /srv/src/ci/docker/run.sh
