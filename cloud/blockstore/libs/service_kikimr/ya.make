LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    auth_provider_kikimr.cpp
    service_kikimr.cpp
)

PEERDIR(
    cloud/blockstore/config

    cloud/blockstore/libs/common
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/service
    cloud/blockstore/libs/storage/api
    cloud/blockstore/libs/storage/core

    library/cpp/actors/core
)

END()

RECURSE_FOR_TESTS(ut)
