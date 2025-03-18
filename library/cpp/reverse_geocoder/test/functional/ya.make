UNITTEST()

OWNER(avitella)

PEERDIR(
    library/cpp/reverse_geocoder/core
    library/cpp/reverse_geocoder/generator
    library/cpp/reverse_geocoder/open_street_map
    library/cpp/reverse_geocoder/test/unittest
)

DATA(
    sbr://117283990 # andorra-latest.osm.pbf
    sbr://117353370 # andorra-latest.dat.pre
)

SRCS(
    converter.cpp
    generator.cpp
    geo_data_map.cpp
    lookup.cpp
    parser.cpp
)

SIZE(MEDIUM)

END()
