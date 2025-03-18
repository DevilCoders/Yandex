#!/usr/bin/env bash
set -e -o pipefail

./learn < sample_docs.protobin > learned_freqs.txt
