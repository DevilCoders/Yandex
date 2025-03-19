PROGRAM(blockstore-disk-agent)

OWNER(g:cloud-nbs)

IF (PROFILE_MEMORY_ALLOCATIONS)
    ALLOCATOR(LF_DBG)
ELSE()
    ALLOCATOR(TCMALLOC_TC)
ENDIF()

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/disk_agent/sources.inc)

PEERDIR(
    ydb/core/blobstorage/lwtrace_probes
    ydb/core/protos
    ydb/core/tablet_flat
)

END()

RECURSE_FOR_TESTS(
    lfdbg
    ut
)
