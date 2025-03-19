LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    listener.cpp
    service.cpp
)

PEERDIR(
    cloud/filestore/libs/service

    cloud/storage/core/libs/common
    cloud/storage/core/libs/coroutine
    cloud/storage/core/libs/diagnostics
)

END()

RECURSE_FOR_TESTS(ut)
