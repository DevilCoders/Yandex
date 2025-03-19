LIBRARY()

OWNER(
    g:base
    rymis
)

PEERDIR(
    kernel/doom/hits
    kernel/doom/wad
    kernel/doom/offroad_doc_wad
    library/cpp/offroad/codec
    library/cpp/offroad/custom
    library/cpp/offroad/streams
    library/cpp/offroad/tuple
    library/cpp/offroad/utility
)

SRCS(
    offroad_sent_wad_io.h
    sent_hit_adaptors.h
)

END()
