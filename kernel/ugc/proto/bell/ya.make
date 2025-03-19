OWNER(
    g:sup
)

PROTO_LIBRARY()

SRCS(
    block.proto
    context.proto
    global.proto
    message.proto
    message_pack.proto
    multi_message.proto
    notification_group.proto
    notification_type.proto
    service.proto
    ticker.proto
    user.proto
    user_service.proto
)

PEERDIR(
    kernel/ugc/schema/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
