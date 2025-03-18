PROGRAM()

OWNER(
    pagein
)

SRCS(
    main.cpp
)

GENERATE_ENUM_SERIALIZATION(factor_domains.h)

PEERDIR(
    mapreduce/yt/client
    library/cpp/getopt
    kernel/snippets/factors
)

END()
