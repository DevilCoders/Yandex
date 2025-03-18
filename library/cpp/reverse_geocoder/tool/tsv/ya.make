LIBRARY()

OWNER(g:geotargeting)

PEERDIR(
    contrib/libs/geos
    library/cpp/reverse_geocoder/proto
    library/cpp/reverse_geocoder/proto_library
)

SRCS(
    convert.cpp
)

END()
