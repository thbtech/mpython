#!/usr/bin/env sh

set -x

git submodule update --init lib/berkeley-db-1.xx
make ${MAKEOPTS} -C mpy-cross
make ${MAKEOPTS} -C ports/esp32 ESPIDF=$IDF_PATH
