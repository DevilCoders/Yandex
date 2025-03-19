#!/bin/bash

set -x
set -e

if [ -d "$1" ]; then
    echo "change dir to $1"
    cd "$1"
fi

if ! [ -f wix.files ]; then
        echo "wix.files not found"
        exit 1
fi

if ! [ -f wix.json ]; then
        echo "wix.json not found"
        exit 1
fi

tmpdir=$(mktemp -d /tmp/wix.XXXXXX)
mkdir $tmpdir/src

SRCDIR=$tmpdir/src
if svn info > /dev/null 2>&1; then
    SCM_REVISION=1.$(svn info | grep ^Revision: | awk '{print $2}')
elif git status > /dev/null 2>&1; then
    SCM_REVISION=$(git rev-list HEAD --count).$(git rev-parse --short HEAD | perl -ne 'print hex $_')
else
    echo "neither git nor svn directory"
    exit 1
fi
MSI_VERSION=${MSI_VERSION:-$SCM_REVISION}

eval "cat <<EOF
$(<wix.json)
EOF
" 2>/dev/null >$tmpdir/wix.json

while IFS=" " read -ra pair; do
        mkdir -p $tmpdir/src/$(dirname ${pair[1]})
        cp ${pair[0]} $tmpdir/src/${pair[1]}
done < wix.files

docker run -v $tmpdir:/package registry.yandex.net/dbaas/msi-builder

cp $tmpdir/*.msi .

rm -rf $tmpdir
