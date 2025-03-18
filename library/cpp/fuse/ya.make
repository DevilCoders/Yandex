LIBRARY()

OWNER(g:arc)

CXXFLAGS(-Wimplicit-fallthrough)

SRCS(
    dispatcher.cpp
    fd_channel.cpp
    fuse_dir_list.cpp
    fuse_get_version.cpp
    fuse_mount.cpp
    fuse_xattr_buf.cpp
    fuse_xattr_list.cpp
    low_mount.cpp
    pid_ring.cpp
    raw_channel.cpp
    request_context.cpp
)

PEERDIR(
    library/cpp/fuse/deadline
    library/cpp/fuse/mrw_lock
    library/cpp/fuse/socket_pair
    library/cpp/logger
    library/cpp/monlib/dynamic_counters
    library/cpp/threading/future
    contrib/libs/linux-headers
)

IF (OS_DARWIN)
    LDFLAGS(
        -framework CoreFoundation
        -framework DiskArbitration
    )
    PEERDIR(
        contrib/libs/macfuse-headers
    )
ENDIF()

GENERATE_ENUM_SERIALIZATION(channel.h)

END()

RECURSE(
    deadline
    mrw_lock
    loop
)
