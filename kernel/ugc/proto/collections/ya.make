OWNER(
    g:ugc
    g:collections-backend
    stakanviski
)

PROTO_LIBRARY()

SRCS(
    board.proto
    card.proto
    common.proto
    competition_result.proto
    content.proto
    meta_info.proto
    origin.proto
    request_wizard_judgement.proto
    source_meta.proto
    sources.proto
)

PEERDIR(
    kernel/ugc/schema/proto
)


EXCLUDE_TAGS(GO_PROTO)

END()
