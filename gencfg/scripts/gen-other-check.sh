#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

# check fo syntax validity
/skynet/python/bin/python -m py_compile `svn list -R |grep "\.py$"`
