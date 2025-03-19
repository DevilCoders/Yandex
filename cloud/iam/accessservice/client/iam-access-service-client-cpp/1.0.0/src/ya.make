LIBRARY()
VERSION(1.0.0)
PROVIDES(iam-access-service-client-cpp)

OWNER(g:cloud-iam)

PEERDIR(
    cloud/iam/accessservice/client/iam-access-service-client-cpp/1.0.0/submodules/iam-access-service-client-proto/private-api/yandex/cloud/priv/accessservice/v2
)

SRCS(
    client.cpp
    credentials.cpp
    convert.cpp
    resource.cpp
    subject.cpp
)

CFLAGS(-DARCADIA_BUILD)

USE_COMMON_GOOGLE_APIS(
    api/annotations
    rpc/code
    rpc/errdetails
    rpc/status
    type/timeofday
    type/dayofweek
)

END()
