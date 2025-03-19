LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/geometry/protos
)

SRCS(
    attachment.proto
    background.proto
    common.proto
    migration.proto
)

END()
