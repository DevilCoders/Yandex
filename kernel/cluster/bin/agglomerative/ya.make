PROGRAM()

OWNER(
    alex-sh
)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/getopt
    kernel/cluster/lib/agglomerative
    kernel/cluster/lib/cluster_metrics
)

ALLOCATOR(LF)

END()
