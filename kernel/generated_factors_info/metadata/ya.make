LIBRARY()

OWNER(
    alsafr
    g:base
    g:factordev
)

SRCS(
    factors_metadata.proto
)

PEERDIR(
    kernel/factors_codegen
)

GENERATE_ENUM_SERIALIZATION(factors_metadata.pb.h)

END()
