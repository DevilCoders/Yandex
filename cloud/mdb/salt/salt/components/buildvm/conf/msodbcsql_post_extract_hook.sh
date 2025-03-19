#!/usr/bin/env bash
set -e
set -x

# 17
if [ -d msodbcsql17 ]; then
    # get rid of EULA_CHECK and version checks, 
    rm -f msodbcsql17/DEBIAN/preinst
    rm -f msodbcsql17/DEBIAN/templates
    rm -f msodbcsql17/DEBIAN/config
fi
