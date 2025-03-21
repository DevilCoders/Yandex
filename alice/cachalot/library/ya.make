OWNER(g:voicetech-infra)

LIBRARY()

SRCS(
    cachalot.cpp
    debug.cpp
    golovan.cpp
    main_class.cpp
    modules/activation/ana_log.cpp
    modules/activation/common.cpp
    modules/activation/request.cpp
    modules/activation/service.cpp
    modules/activation/storage.cpp
    modules/activation/yql_requests.cpp
    modules/cache/key_converter.cpp
    modules/cache/request.cpp
    modules/cache/request_factory.cpp
    modules/cache/service.cpp
    modules/cache/storage.cpp
    modules/cache/storage_tag_to_context.cpp
    modules/gdpr/request.cpp
    modules/gdpr/service.cpp
    modules/gdpr/storage.cpp
    modules/megamind_session/service.cpp
    modules/megamind_session/storage.cpp
    modules/stats/service.cpp
    modules/takeout/request.cpp
    modules/takeout/service.cpp
    modules/takeout/storage.cpp
    modules/vins_context/request.cpp
    modules/vins_context/service.cpp
    modules/vins_context/storage.cpp
    modules/yabio_context/request.cpp
    modules/yabio_context/service.cpp
    modules/yabio_context/storage.cpp
    request.cpp
    service.cpp
    status.cpp
    storage/inmemory/fifo_policy.cpp
    storage/inmemory/imdb.cpp
    storage/inmemory/lfu_policy.cpp
    storage/inmemory/lru_policy.cpp
    storage/inmemory/ordered_policy_base.cpp
    storage/inmemory/read_once_policy.cpp
    storage/inmemory/utils.cpp
    storage/mock.cpp
    storage/redis.cpp
    storage/stats.cpp
    storage/storage.cpp
    storage/ydb.cpp
    storage/ydb_operation.cpp
    utils.cpp
)

PEERDIR(
    alice/cachalot/api/protos
    alice/cachalot/library/config
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/cuttlefish/common
    alice/cachalot/events
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/metrics
    alice/cuttlefish/library/mlock
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/redis
    apphost/api/service/cpp
    library/cpp/blockcodecs/codecs/lz4
    library/cpp/blockcodecs/core
    library/cpp/json
    library/cpp/proto_config
    library/cpp/protobuf/interop
    library/cpp/threading/future/subscription
    library/cpp/unistat
    voicetech/library/itags
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
    ydb/public/sdk/cpp/client/ydb_types
)

END()

RECURSE(
    config
)

RECURSE_FOR_TESTS(
    ut
)
