#!/bin/bash

set -e

if [ ! -z ${TEST_CONFIG_PATH} ]; then
    echo "Test will be launched with config: ${TEST_CONFIG_PATH}"
fi

echo > coverage.txt

for pkg in "$@"
do
    go test -v -coverprofile=profile.out -covermode=atomic -coverpkg=$(./coverpkg.sh ${pkg}) ${pkg}
    if [ -f profile.out ]; then
		cat profile.out >> coverage.txt
        rm profile.out
	fi
done

sed -ie '2!s/mode: atomic//;/^$/d' coverage.txt
