LIBRARY()

OWNER(
    aavdonkin
)

SRCS(
    advmerger.cpp
    hitbuffermanager.cpp
    hititeratorheap.cpp
    keys.cpp
    lfproc.cpp
    yxformbuf.cpp
)

PEERDIR(
    kernel/keyinv/hitlist
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    kernel/search_types
    kernel/sent_lens
    library/cpp/containers/mh_heap
    library/cpp/deprecated/autoarray
    library/cpp/deprecated/fgood
    library/cpp/sorter
    library/cpp/wordpos
)

END()
