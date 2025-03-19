#!/bin/sh

set -x
set -e

if [ -d "$1" ]; then
    echo "change dir to $1"
    cd "$1"
fi

SLEET_CONFIG=${SLEET_CONFIG:-/etc/sleet.json}
NUPKG_BUILDER_IMAGE=${NUPKG_BUILDER_IMAGE:-registry.yandex.net/dbaas/nupkg-builder:latest}

for pkg in *.nupkg
do
    docker run --rm -v `pwd`:/package -v $SLEET_CONFIG:/etc/sleet.json $NUPKG_BUILDER_IMAGE upload -f --verbose $pkg
done
