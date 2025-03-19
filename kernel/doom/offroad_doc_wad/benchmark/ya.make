Y_BENCHMARK()

OWNER(
    g:base
    sskvor
)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/doom/chunked_wad
    kernel/doom/enums
    kernel/doom/hits
    kernel/doom/info
    kernel/doom/key
    kernel/doom/offroad_doc_wad
    kernel/keyinv/invkeypos
    kernel/search_types
    library/cpp/offroad/codec
    library/cpp/offroad/custom
    library/cpp/offroad/streams
    library/cpp/offroad/sub
    library/cpp/offroad/tuple
    library/cpp/offroad/utility
    library/cpp/offroad/wad
)

END()
