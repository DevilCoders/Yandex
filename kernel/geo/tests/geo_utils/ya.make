PROGRAM(geo_utils_test)

OWNER(akhropov)

PEERDIR(
    kernel/geo
    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()

RECURSE(tests)
