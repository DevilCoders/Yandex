LIBRARY()

OWNER(
    alsafr
    nkmakarov
    g:base
    g:factordev
)

SRCS(
    factors_codegen.cpp
)

PEERDIR(
    kernel/factor_slices
    kernel/proto_codegen
    library/cpp/protobuf/json
)

END()
