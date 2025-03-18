#!/usr/bin/env bash

mkdir -p .cache
./kiwi_unpacker -f sample_docs.protobin -d .cache -p 30 | pv -l > url_groups.txt
