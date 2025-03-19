PY3TEST()

OWNER(g:cloud-nbs)

SRCDIR(${ARCADIA_ROOT}/cloud/blockstore/tests/loadtest/local-nonrepl)
INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/loadtest/local-nonrepl/sources.inc)

ENV(NBS_ALLOC="lfdbg")
ENV(DEDICATED_DISK_AGENT="true")

DEPENDS(
    cloud/blockstore/daemon/lfdbg
    cloud/blockstore/disk_agent/lfdbg
)

END()
