PROGRAM(server_template)

OWNER(g:fintech-csdev)

ALLOCATOR(LF)

PEERDIR(
    kernel/daemon
    kernel/common_server/solutions/server_template/src/server
    mapreduce/yt/interface
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

