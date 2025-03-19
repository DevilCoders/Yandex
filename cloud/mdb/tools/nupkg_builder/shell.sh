#!/bin/sh

set -x
set -e

SLEET_CONFIG=${SLEET_CONFIG:-/etc/sleet.json}

docker run -it --rm -v `pwd`:/package -v $SLEET_CONFIG:/etc/sleet.json registry.yandex.net/dbaas/nupkg-builder:latest shell
