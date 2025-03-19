#!/bin/bash

if [ ${USE_ENV_TOOLS:-0} -ne 0 ]; then
    echo env tools will be used
    exit 0
fi

set -eu

cd `dirname $0`

BINDIR=../modules/.bin
if [ -d "${BINDIR}" ] && [ "`ls ${BINDIR}`" ]; then
    exit 0
fi

ya package --tar --raw-package --raw-package-path=${BINDIR} ./pkg/pkg.json
