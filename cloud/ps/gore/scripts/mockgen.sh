#!/bin/bash

set -eu

OWNER="${OWNER:-g:cloud-ps}"

search_path=$(pwd)
while [ "$search_path" != "/" ] ; do
    search_path=$(dirname "$search_path")
    if test -e "$search_path/ya"; then
        ARCADIA_ROOT=${search_path}
        break
    fi;
done

echo "Generating mock for $1..."
"${ARCADIA_ROOT}/ya" tool mockgen -destination=mocks/mocks.go -package=mocks . "$1"

echo "Mocks generated"
echo "Executing 'yo' to fix ya.make files..."
"${ARCADIA_ROOT}/ya" tool yo fix -add-owner="$OWNER" .

echo "Done"
