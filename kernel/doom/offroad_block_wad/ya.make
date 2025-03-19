LIBRARY()

OWNER(
    g:base
)

SRCS(
    offroad_block_wad_io.h
    offroad_block_wad_reader.h
    offroad_block_wad_sampler.h
    offroad_block_wad_searcher.h
    offroad_block_wad_writer.h
)

PEERDIR(
    kernel/doom/hits
    kernel/doom/offroad_doc_wad
    kernel/doom/wad
    library/cpp/offroad/custom
    library/cpp/offroad/flat
)

END()
