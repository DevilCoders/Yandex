PROGRAM(tm_print)

OWNER(
    gotmanov
)

PEERDIR(
    kernel/idx_proto
    kernel/reqbundle
    kernel/text_machine/interface
    kernel/text_machine/proto
    kernel/text_machine/util
    library/cpp/getopt
    library/cpp/streams/factory
    library/cpp/string_utils/base64
    search/tools/idx_proto
)

SRCS(
    main.cpp
    mode_hits_map.cpp
    hits_map.cpp
)

GENERATE_ENUM_SERIALIZATION(hits_map.h)
GENERATE_ENUM_SERIALIZATION(mode_hits_map.h)

END()
