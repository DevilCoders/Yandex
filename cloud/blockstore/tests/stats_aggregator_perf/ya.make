Y_BENCHMARK()

OWNER(g:cloud-nbs)

IF (PROFILE_MEMORY_ALLOCATIONS)
    ALLOCATOR(LF_DBG)
ELSE()
    ALLOCATOR(TCMALLOC_TC)
ENDIF()

SRCS(
    main.cpp
)

PEERDIR(
    cloud/blockstore/libs/diagnostics

    library/cpp/getopt
    library/cpp/logger
    library/cpp/resource
    library/cpp/sighandler
)

RESOURCE(
    res/client_stats.json           client_stats
    res/client_volume_stats.json    client_volume_stats
)

END()
