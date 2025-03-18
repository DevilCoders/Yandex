UNITTEST_FOR(library/cpp/reverse_geocoder/core)

OWNER(avitella)

PEERDIR(
    library/cpp/reverse_geocoder/test/unittest
)

SRCS(
    area_box_ut.cpp
    bbox_ut.cpp
    common_ut.cpp
    location_ut.cpp
    point_ut.cpp
    region_ut.cpp
    edge_ut.cpp
    part_ut.cpp
)

END()
