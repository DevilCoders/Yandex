LIBRARY()

OWNER(
    elric
    g:base
)

PEERDIR(
    kernel/doom/chunked_wad
    kernel/doom/enums
    kernel/doom/hits
    kernel/doom/info
    kernel/doom/key
    kernel/doom/offroad_wad/proto
    kernel/doom/wad
    kernel/doom/search_fetcher
    kernel/indexann/interface
    kernel/keyinv/invkeypos
    kernel/search_types
    kernel/searchlog
    library/cpp/offroad/codec
    library/cpp/offroad/custom
    library/cpp/offroad/keyinv
    library/cpp/offroad/streams
    library/cpp/offroad/sub
    library/cpp/offroad/tuple
    library/cpp/offroad/utility
    library/cpp/offroad/wad
)

SRCS()

END()

RECURSE(
    proto
)