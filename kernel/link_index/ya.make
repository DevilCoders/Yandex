LIBRARY()

OWNER(g:jupiter)

PEERDIR(
    kernel/xref
    library/cpp/deprecated/calc_module
    library/cpp/protobuf/protofile
    library/cpp/string_utils/url
    yweb/protos
)

SRCS(
    clampticandmatches.cpp
)

END()
