G_BENCHMARK()

OWNER(g:factordev)

SRCS(
    bench.cpp
)

FROM_SANDBOX(FILE 1205994907 OUT texts)

RESOURCE(
    texts /texts
)

PEERDIR(
    kernel/search_query
    library/cpp/resource
)

END()
