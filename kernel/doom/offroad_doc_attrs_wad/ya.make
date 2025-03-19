LIBRARY()

OWNER(g:base)

PEERDIR(
    kernel/doom/enums
    kernel/doom/hits
    kernel/doom/info
    kernel/doom/offroad_common
    kernel/doom/offroad_doc_wad
    kernel/doom/wad
    library/cpp/offroad/codec
    library/cpp/offroad/custom
    library/cpp/offroad/streams
    library/cpp/offroad/sub
    library/cpp/offroad/tuple
    library/cpp/offroad/utility
    library/cpp/offroad/wad
)

SRCS(
    doc_attrs_hit_adaptors.h
    offroad_doc_attrs_wad_io.h
)

END()
