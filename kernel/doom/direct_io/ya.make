LIBRARY()

OWNER(
    sankear
    g:base
)

SRCS(
    async_direct_io_thread_local_reader.h
    direct_io_file_read_request.h
    io_getevents.cpp
)

IF (OS_LINUX)
    PEERDIR(
        contrib/libs/libaio
        library/cpp/deprecated/atomic
    )
ENDIF(OS_LINUX)

END()
