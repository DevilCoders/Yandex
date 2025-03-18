LIBRARY()

OWNER(avitella)

PEERDIR(
    library/cpp/reverse_geocoder/core
    library/cpp/reverse_geocoder/logger
)

SRCS(
    common.cpp
    config.cpp
    gen_geo_data.cpp
    generator.cpp
    handler.cpp
    locations_converter.cpp
    main.cpp
    mut_geo_data.cpp
    raw_handler.cpp
    regions_handler.cpp
    slab_handler.cpp
)

END()
