LIBRARY()

OWNER(
    crossby
    filmih
    g:factordev
    g:neural-search
)

SRCS(
    make_clusters.cpp
    processor.cpp
)

PEERDIR(
    contrib/libs/opencv
    kernel/cluster_machine/proto
)

END()
