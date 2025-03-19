LIBRARY()

OWNER(g:snippets)

SRCS(
    snippets.proto
    secondary_snippets.proto
    raw_preview.proto
)

PEERDIR(
    library/cpp/langmask/proto
    search/idl
)

GENERATE_ENUM_SERIALIZATION(enums.h)

END()
