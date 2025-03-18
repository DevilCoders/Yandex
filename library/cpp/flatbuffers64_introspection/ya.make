OWNER(eeight)

LIBRARY()

PEERDIR(
    library/cpp/getopt
    library/cpp/resource

    contrib/libs/flatbuffers64
)

ADDINCL(contrib/libs/flatbuffers64/include)

SRCS(main.cpp)

END()
