LIBRARY()

OWNER(akhropov)

SRCS(
    qeflexindex_1.cpp
    qeflexindex_1_impl.cpp
    keyconv.cpp
)

PEERDIR(
    kernel/geo
    kernel/index_mapping
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    kernel/search_types
    kernel/searchlog
    library/cpp/charset
    library/cpp/compproto
    library/cpp/packedtypes
    library/cpp/streams/zc_memory_input
)

END()
