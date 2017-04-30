#!/bin/bash

set -ex

: ${build_method:=cmake}

cd /srv/build
case "$build_method" in
  cmake)
    cmake -G Ninja -DCMAKE_BUILD_TYPE="$build_type" /srv/src
    ;;
  meson)
    meson $build_flags --buildtype="$build_type" /srv/src
    ;;
  *)
    echo unknown build_method: "$build_method" >&2
    exit 1
    ;;
esac
ninja-build $targets


