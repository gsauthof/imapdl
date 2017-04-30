#!/bin/bash

set -ex

. "${BASH_SOURCE%/*}"/config.sh

if [[ $build_flags == *coverage* || $CXXFLAGS == *--coverage* || $CFLAGS == *--coverage* ]]; then
  bash <(curl -s https://codecov.io/bash) -f "$build"/coverage.info \
    || echo "Codecov.io reporting failed"
fi
