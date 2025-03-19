LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    notify.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics

    library/cpp/http/client
    library/cpp/threading/future
)

END()

RECURSE_FOR_TESTS(ut)
