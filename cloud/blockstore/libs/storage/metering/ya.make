LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    metering.cpp
)

PEERDIR(
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/storage/api

    library/cpp/actors/core
    library/cpp/logger
)

END()
