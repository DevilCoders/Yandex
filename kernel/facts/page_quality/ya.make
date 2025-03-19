LIBRARY()

OWNER(g:facts)

SRCS(
    pq.cpp
)

PEERDIR(
    kernel/querydata/idl
    kernel/querydata/server
    library/cpp/scheme
)

END()
