#!/bin/bash
set -xeu -o pipefail

RELEASE="${1#v}"

PATCHSET_DIR="$(pwd)/patchset"
test -d "$PATCHSET_DIR"

TEMPDIR="$(mktemp -d)"
FILE="salt-${RELEASE}.tar.gz"
URL="https://github.com/saltstack/salt/releases/download/v$RELEASE/$FILE"
pushd $TEMPDIR
curl -LOL "$URL"
tar xzf "$FILE"
pushd "${FILE%%.tar.gz}"
find "$PATCHSET_DIR" -type f -a -name "*.patch" |xargs -t --no-run-if-empty -I{} bash -c 'patch -p0 < {}'
popd
mv $FILE "${FILE}.origin"
tar czf "$FILE" "${FILE%%.tar.gz}"
ya upload --ttl=inf $FILE
rm -v "$FILE" "${FILE}.origin"
rm -rf "$TEMPDIR/salt-$RELEASE"
popd
rmdir -v "$TEMPDIR"
