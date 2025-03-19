OWNER(g:cloud-kms)

LIBRARY(kms-client-cpp)

PEERDIR(
    contrib/libs/grpc
    cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1
    library/cpp/deprecated/atomic
)

SRCS(
    kmsclient.cpp
)

END()

RECURSE(
    example
)
