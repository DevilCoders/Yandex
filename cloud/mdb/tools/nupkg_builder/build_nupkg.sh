#!/bin/bash

set -x
set -e

if [ -d "$1" ]; then
    echo "change dir to $1"
    cd "$1"
fi

if ! [ -f package.nuspec ]; then
        echo "package.nuspec not found"
        exit 1
fi

if [ -z "$NUPKG_VERSION" ]; then
    if svn info > /dev/null 2>&1; then
        NUPKG_VERSION=1.$(svn info | grep ^Revision: | awk '{print $2}')
    elif git status > /dev/null 2>&1; then
        NUPKG_VERSION=$(git rev-list HEAD --count).$(git rev-parse --short HEAD | perl -ne 'print hex $_')
    else
        echo "neither git nor svn directory"
        exit 1
    fi
fi
echo $NUPKG_VERSION

NUPKG_BUILDER_IMAGE=${NUPKG_BUILDER_IMAGE:-registry.yandex.net/dbaas/nupkg-builder:latest}

tmpspec=$(mktemp /tmp/nuspec.XXXXXX)

eval "cat <<EOF
$(<package.nuspec)
EOF
" 2>/dev/null > $tmpspec

docker run --rm -v `pwd`:/package -v $tmpspec:/package/package.nuspec $NUPKG_BUILDER_IMAGE build

rm -rf $tmpspec
