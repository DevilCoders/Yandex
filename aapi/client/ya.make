LIBRARY()

OWNER(g:aapi)

PEERDIR(
    aapi/lib/store
    aapi/lib/proto
    aapi/lib/common
    library/cpp/blockcodecs
    library/cpp/deprecated/atomic
)

SRCS(
    client.cpp
)

END()
