PROGRAM(blockstore-server)

OWNER(g:cloud-nbs)

IF (PROFILE_MEMORY_ALLOCATIONS)
    ALLOCATOR(LF_DBG)
ELSE()
    ALLOCATOR(TCMALLOC_TC)
ENDIF()

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/daemon/sources.inc)

PEERDIR(
    cloud/blockstore/libs/daemon
)

END()

RECURSE_FOR_TESTS(
    lfdbg
)
