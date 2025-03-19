PROGRAM(fallback_proxy)

OWNER(g:fintech-csdev)

ALLOCATOR(LF)

PEERDIR(
    kernel/common_server/solutions/fallback_proxy/src/server
)
IF (NOT OS_WINDOWS)
    PEERDIR(
        kernel/common_server/library/storage/postgres
    )
ENDIF()

SRCS(
    main.cpp
)

END()

