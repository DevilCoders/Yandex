LIBRARY()

OWNER(
    avitella
    dieash
)

PEERDIR(
    library/cpp/getopt/small
    library/cpp/reverse_geocoder/core
    library/cpp/reverse_geocoder/library
)

SRCS(
    extractor.cpp
    main.cpp
)

END()
