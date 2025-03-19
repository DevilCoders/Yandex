#!/bin/bash

set -ex

cd "$(dirname "$0")"

# 1. Install grpcio-tools
# Pass option --no-binary :all: for compatibility with macOS running on M1 chip
# See issue https://github.com/grpc/grpc/issues/28387
pip install --no-binary :all: grpcio-tools==1.46.3

# 2. Checkout latest version of googleapis proto specs
if pushd googleapis; then git pull; popd || exit; else git clone https://github.com/googleapis/googleapis.git googleapis; fi

# 3. Compile Yandex Cloud's proto specs from /cloud/bitbucket/
python3 tools/protoc.py
