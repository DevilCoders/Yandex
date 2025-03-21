#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

../../granet \
    normalize \
    --input demo_tr.tsv \
    --lang tr \
    --type original,basic,granet,fst \
    --column 0 \
    --header
