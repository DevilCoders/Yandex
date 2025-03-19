PY3TEST()

OWNER(g:cloud-nbs)

SRCDIR(${ARCADIA_ROOT}/cloud/blockstore/tests/loadtest/local-nonrepl)
INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/loadtest/local-nonrepl/sources.inc)

ENV(DEDICATED_DISK_AGENT="true")

DEPENDS(
    cloud/blockstore/daemon
    cloud/blockstore/disk_agent
)

END()
