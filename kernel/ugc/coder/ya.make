LIBRARY()

OWNER(g:ugc)

PEERDIR(
    kernel/signurl
    kernel/ugc/aggregation
    kernel/ugc/proto
    library/cpp/scheme
)

SRCS(
    coder.cpp
)

END()
