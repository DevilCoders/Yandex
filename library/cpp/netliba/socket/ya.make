LIBRARY()

OWNER(
    dcherednik
    gulin
    g:yamr
)

IF (MUSL)
    # TODO
    NO_COMPILER_WARNINGS()
ENDIF()

PEERDIR(
    contrib/libs/libc_compat
    library/cpp/threading/chunk_queue
    library/cpp/deprecated/atomic
)

SRCS(
    creators.cpp
    socket.cpp
    stdafx.cpp
)

END()
