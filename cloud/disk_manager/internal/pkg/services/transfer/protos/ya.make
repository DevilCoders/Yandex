OWNER(g:cloud-nbs)

PROTO_LIBRARY()

INCLUDE_TAGS(GO_PROTO)
EXCLUDE_TAGS(JAVA_PROTO)

SRCS(
    transfer_from_image_to_disk_task.proto
    transfer_from_snapshot_to_disk_task.proto
)

PEERDIR(
    cloud/disk_manager/internal/pkg/types
)

END()
