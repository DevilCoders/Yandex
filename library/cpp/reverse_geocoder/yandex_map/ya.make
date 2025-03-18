LIBRARY()

OWNER(avitella)

PEERDIR(
    library/cpp/reverse_geocoder/core
    library/cpp/reverse_geocoder/logger
    library/cpp/geobase
    library/cpp/xml/document
    library/cpp/regex/pcre
)

SRCS(
    converter.cpp
    geomapping.cpp
    polygon_parser.cpp
    toponym_checker.cpp
    toponym_traits.cpp
)

GENERATE_ENUM_SERIALIZATION(polygon_parser.h)

END()
