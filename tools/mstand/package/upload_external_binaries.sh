#!/usr/bin/env bash

set -e

mkdir libra_files
cd libra_files
../download-external-binaries.sh
cd ..

ya upload libra_files --ttl inf

rm -rf libra_files
