LIBRARY()

OWNER(
    g:base
)

SRCS(
    offroad_hashed_keyinv_wad_io.h
    offroad_hashed_keyinv_wad_reader.h
    offroad_hashed_keyinv_wad_sampler.h
    offroad_hashed_keyinv_wad_searcher.h
    offroad_hashed_keyinv_wad_writer.h
)

PEERDIR(
    kernel/doom/hits
    kernel/doom/offroad_doc_wad
    kernel/doom/wad
    kernel/doom/offroad_block_wad
    library/cpp/offroad/custom
    library/cpp/offroad/flat
)

END()
