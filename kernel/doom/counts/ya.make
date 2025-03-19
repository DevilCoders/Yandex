LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    counts_memory_index_reader.h
    dummy.cpp
)

PEERDIR(
    kernel/doom/hits
    kernel/doom/progress
    search/panther/mappings
    search/panther/indexing/operations
)

END()
