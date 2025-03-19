LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    from_rt_index_reader.h
    dummy.cpp
)

PEERDIR(
    kernel/doom/adaptors
    kernel/doom/hits
    yweb/realtime/indexer/common/indexing
)

END()
