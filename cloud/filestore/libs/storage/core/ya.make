LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    model.cpp
    probes.cpp
    request_info.cpp
    tablet.cpp
    tablet_counters.cpp
    tablet_schema.cpp
    utils.cpp
)

PEERDIR(
    cloud/filestore/config
    cloud/filestore/libs/service
    cloud/filestore/public/api/protos
    cloud/storage/core/libs/common
    library/cpp/actors/core
    library/cpp/actors/wilson
    library/cpp/lwtrace
    ydb/core/base
    ydb/core/engine/minikql
    ydb/core/protos
    ydb/core/tablet
    ydb/core/tablet_flat
    ydb/library/yql/sql/pg_dummy
    library/cpp/deprecated/atomic
)

END()
