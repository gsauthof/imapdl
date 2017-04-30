#!/bin/bash

set -ex


# Docker 17.03 has the --env option, e.g.
# --env CMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
# but Travis currently is at 1.12.3 which doesn't have it

docker exec devel env \
  CFLAGS="$CFLAGS" \
  CXXFLAGS="$CXXFLAGS" \
  LDFLAGS="$LDFLAGS" \
  build_method="$build_method" \
  build_type="$build_type" \
  build_flags="$build_flags" \
  targets="$targets" \
  /srv/src/ci/docker/build.sh
