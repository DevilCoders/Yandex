LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    authorizer.cpp
    bootstrapper.cpp
    disk_agent.cpp
    disk_registry.cpp
    disk_registry_proxy.cpp
    metering.cpp
    partition.cpp
    partition2.cpp
    service.cpp
    ss_proxy.cpp
    stats_service.cpp
    undelivered.cpp
    user_stats.cpp
    volume.cpp
    volume_balancer.cpp
    volume_proxy.cpp
)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/libs/common
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/service
    cloud/blockstore/libs/storage/protos
    cloud/blockstore/private/api/protos
    cloud/blockstore/public/api/protos
    library/cpp/actors/core
    ydb/core/base
    ydb/core/protos
)

END()
