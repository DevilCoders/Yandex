LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    logbroker.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics

    kikimr/persqueue/sdk/deprecated/cpp/v2
    kikimr/public/sdk/cpp/client/iam

    library/cpp/monlib/service/pages
    library/cpp/threading/future
)

END()

RECURSE_FOR_TESTS(ut)
