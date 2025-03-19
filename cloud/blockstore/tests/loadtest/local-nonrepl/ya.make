PY3TEST()

OWNER(g:cloud-nbs)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/loadtest/local-nonrepl/sources.inc)

DEPENDS(
    cloud/blockstore/daemon
)

END()

RECURSE_FOR_TESTS(
    dedicated
    dedicated-lfdbg
    lfdbg
)
