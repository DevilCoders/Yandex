OWNER(yazevnul)

PROGRAM()

SRCS(
    main.cpp
    mode_load.cpp
    mode_random.cpp
)

PEERDIR(
    library/cpp/accurate_accumulate
    library/cpp/getopt/small
    library/cpp/sampling
    library/cpp/streams/factory
)

END()
