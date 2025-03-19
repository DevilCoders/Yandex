LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    actorsystem.cpp
    diskagent_actorsystem.cpp
    node.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/rdma
    cloud/blockstore/libs/spdk
    cloud/blockstore/libs/storage/api
    cloud/blockstore/libs/storage/auth
    cloud/blockstore/libs/storage/disk_agent
    cloud/blockstore/libs/storage/disk_registry
    cloud/blockstore/libs/storage/disk_registry_proxy
    cloud/blockstore/libs/storage/metering
    cloud/blockstore/libs/storage/partition
    cloud/blockstore/libs/storage/partition2
    cloud/blockstore/libs/storage/service
    cloud/blockstore/libs/storage/stats_service
    cloud/blockstore/libs/storage/ss_proxy
    cloud/blockstore/libs/storage/undelivered
    cloud/blockstore/libs/storage/user_stats
    cloud/blockstore/libs/storage/volume
    cloud/blockstore/libs/storage/volume_balancer
    cloud/blockstore/libs/storage/volume_proxy
    cloud/storage/core/libs/hive_proxy
    kikimr/yndx/keys
    kikimr/yndx/security
    library/cpp/actors/core
    library/cpp/actors/util
    library/cpp/lwtrace/mon
    ydb/core/base
    ydb/core/blobstorage/testload
    ydb/core/driver_lib/run
    ydb/core/mind
    ydb/core/mon
    ydb/core/protos
    ydb/core/tablet
    ydb/public/lib/deprecated/kicli
)

YQL_LAST_ABI_VERSION()

END()
