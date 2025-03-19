PY3TEST()

OWNER(g:cloud-nbs)

SRCDIR(${ARCADIA_ROOT}/cloud/blockstore/tests/loadtest/local-nonrepl)
INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/loadtest/local-nonrepl/sources.inc)

ENV(NBS_ALLOC="lfdbg")

DEPENDS(
    cloud/blockstore/daemon/lfdbg
)

END()
