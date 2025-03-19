LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    storage_local.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics

    library/cpp/monlib/service/pages
    library/cpp/threading/future
)

END()

RECURSE_FOR_TESTS(ut)
