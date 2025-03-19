LIBRARY()

OWNER(
    g:base
)

SRCS(
    detect_index_format.cpp
    index_info.cpp
    index_info.proto
    index_format.h
    index_format_usage.cpp
    info_index_reader.h
    info_index_writer.h
)

GENERATE_ENUM_SERIALIZATION(index_format.h)

PEERDIR(
    library/cpp/protobuf/json
)

END()
