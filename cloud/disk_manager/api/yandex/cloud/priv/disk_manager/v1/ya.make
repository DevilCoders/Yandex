OWNER(g:cloud-nbs)

PROTO_LIBRARY()

GRPC()
ONLY_TAGS(GO_PROTO)

USE_COMMON_GOOGLE_APIS()

SRCS(
    disk.proto
    disk_service.proto
    error.proto
    image.proto
    image_service.proto
    filesystem_service.proto
    operation_service.proto
    placement_group.proto
    placement_group_service.proto
    snapshot_service.proto
)

PEERDIR(
    cloud/bitbucket/common-api/yandex/cloud/api
    cloud/bitbucket/private-api/yandex/cloud/priv
    cloud/bitbucket/private-api/yandex/cloud/priv/operation
)

PROTO_NAMESPACE(GLOBAL cloud/disk_manager/api)

END()
