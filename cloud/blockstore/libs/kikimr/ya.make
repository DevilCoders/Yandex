LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    components.cpp
    events.cpp
    helpers.cpp
    trace.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/service
    cloud/blockstore/public/api/protos
    cloud/storage/core/libs/kikimr
    library/cpp/actors/core
    library/cpp/actors/wilson
    library/cpp/lwtrace
    ydb/core/base
    ydb/core/protos
)

END()
