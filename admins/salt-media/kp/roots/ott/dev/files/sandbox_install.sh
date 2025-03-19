#!/bin/bash

ARC_PATH='arcadia.yandex.ru/arc/trunk/arcadia/junk/tabroot/script/sandbox-install.sh'
ARC_TRANSPORT='svn+ssh://'

ARC_TMPDIR=$(mktemp -d)

svn checkout "${ARC_TRANSPORT}$(dirname ${ARC_PATH})" "$ARC_TMPDIR" --depth empty
(
    cd "$ARC_TMPDIR"
    svn up $(basename "$ARC_PATH")
)

chmod +x "${ARC_TMPDIR}/$(basename ${ARC_PATH})"

exec "${ARC_TMPDIR}/$(basename ${ARC_PATH})" $@
