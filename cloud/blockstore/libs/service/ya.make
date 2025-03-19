LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    auth_provider.cpp
    auth_scheme.cpp
    context.cpp
    device_handler.cpp
    request.cpp
    request_helpers.cpp
    service.cpp
    service_auth.cpp
    service_null.cpp
    service_test.cpp
    storage.cpp
    storage_provider.cpp
    storage_test.cpp
)

PEERDIR(
    cloud/blockstore/public/api/protos
    cloud/blockstore/config

    cloud/blockstore/libs/common

    library/cpp/lwtrace
)

END()

RECURSE_FOR_TESTS(ut)
