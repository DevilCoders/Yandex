PROGRAM(reverse-geocoder)

OWNER(avitella)

ALLOCATOR(J)

PEERDIR(
    library/cpp/reverse_geocoder/core
    library/cpp/reverse_geocoder/generator
    library/cpp/reverse_geocoder/open_street_map
    library/cpp/reverse_geocoder/library
    library/cpp/reverse_geocoder/proto_library
    library/cpp/reverse_geocoder/yandex_map
    library/cpp/reverse_geocoder/tool/benchmark
    library/cpp/reverse_geocoder/tool/border
    library/cpp/reverse_geocoder/tool/geodata4
    library/cpp/reverse_geocoder/tool/lookup
    library/cpp/reverse_geocoder/tool/stat
    library/cpp/reverse_geocoder/tool/tsv
    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()
