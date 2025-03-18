LIBRARY()

OWNER(akastornov)

PEERDIR(
    aapi/lib/store
    aapi/lib/proto
    aapi/lib/node
    aapi/lib/common
    library/cpp/blockcodecs
    library/cpp/json
    library/cpp/threading/future
)

IF (OS_DARWIN)
    PEERDIR(contrib/libs/osxfuse)
ELSE()
    PEERDIR(contrib/libs/fuse)
ENDIF()

SRCS(
    file_system.cpp
    file_system_objects.cpp
    interface.cpp
    protocol.cpp
    utils.cpp
)

END()
