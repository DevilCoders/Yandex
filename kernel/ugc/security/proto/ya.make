OWNER(
    g:ugc
    deep
)

PROTO_LIBRARY()

SRCS(
    crypto_bundle.proto
    record_identifier_bundle.proto
    token.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
