LIBRARY()

OWNER(avitella)

PEERDIR(
    contrib/libs/zlib
    library/cpp/reverse_geocoder/core
    library/cpp/reverse_geocoder/open_street_map/proto
    library/cpp/reverse_geocoder/proto_library
)

SRCS(
    converter.cpp
    open_street_map.cpp
    parser.cpp
    reader.cpp
)

END()
