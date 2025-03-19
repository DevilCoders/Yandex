#!/bin/bash

set -eu

OWNER="${OWNER:-g:mdb}"

search_path=$(pwd)
while [ "$search_path" != "/" ] ; do
    search_path=$(dirname "$search_path")
    if test -e "$search_path/ya"; then
        ARCADIA_ROOT=${search_path}
        break
    fi;
done

import_path=${2:-.}
echo "Generating mock for $1 from ${import_path} ..."
"${ARCADIA_ROOT}/ya" tool mockgen -destination=mocks/mocks.go -package=mocks "$import_path" "$1"

echo "Mocks generated"
echo "Executing 'yo' to fix ya.make files..."
"${ARCADIA_ROOT}/ya" tool yo fix -add-owner="$OWNER" .

echo "Done"
