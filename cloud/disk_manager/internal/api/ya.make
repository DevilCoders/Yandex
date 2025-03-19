OWNER(g:cloud-nbs)

PROTO_LIBRARY()

GRPC()
ONLY_TAGS(GO_PROTO)

USE_COMMON_GOOGLE_APIS()

SRCS(
    private_service.proto
)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/operation
    cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1
)

END()
