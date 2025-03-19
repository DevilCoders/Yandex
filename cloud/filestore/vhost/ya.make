PROGRAM(filestore-vhost)

OWNER(g:cloud-nbs)

IF (PROFILE_MEMORY_ALLOCATIONS)
    ALLOCATOR(LF_DBG)
ELSE()
    ALLOCATOR(TCMALLOC_TC)
ENDIF()

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/vhost/sources.inc)

END()

RECURSE_FOR_TESTS(
    lfdbg
)
