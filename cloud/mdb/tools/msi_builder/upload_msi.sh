#!/bin/bash

set -x
set -e

path=$1
if [ -z "$1" ]; then
        echo "path to msi not specified"
        exit 1
fi

file=$(basename -- "$path")
package="${file%%_*}"

s3cmd put $path s3://mdb-windows/repo/$package/$file
