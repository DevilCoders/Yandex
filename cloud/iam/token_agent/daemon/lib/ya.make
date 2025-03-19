LIBRARY()

OWNER(g:cloud-iam)

CFLAGS(-DTBB_PREVIEW_CONCURRENT_LRU_CACHE=1 -D__TBB_NO_IMPLICIT_LINKAGE -DARCADIA_BUILD)

PEERDIR(
    contrib/libs/asio
    contrib/libs/grpc
    contrib/libs/grpc/grpc++_reflection
    contrib/libs/jwt-cpp
    contrib/libs/tbb
    contrib/libs/uuid
    contrib/libs/yaml-cpp

    library/cpp/logger/global
    library/cpp/monlib/dynamic_counters
    library/cpp/sighandler

    cloud/bitbucket/private-api/yandex/cloud/priv
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/infra/tpmagent/v1
)

SRCS(
    http_server/connection.cpp
    http_server/request_handler.cpp
    http_server/request_parser.cpp
    http_server/response.cpp
    http_server/server.cpp
    config.cpp
    group.cpp
    http_token_service.cpp
    iam_token_client.cpp
    iam_token_client_utils.cpp
    logging_interceptor.cpp
    mon.cpp
    role.cpp
    role_cache.cpp
    server.cpp
    soft_tpm.cpp
    token_service.cpp
    tpm_sign.cpp
    updater.cpp
    user.cpp
)

END()
