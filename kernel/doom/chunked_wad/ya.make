LIBRARY()

OWNER(
    g:base
    sankear
)

SRCS(
    chunked_mega_wad.cpp
    chunked_wad.cpp
    doc_chunk_mapping.cpp
    doc_chunk_mapping_searcher.h
    doc_chunk_mapping_writer.h
    single_chunked_wad.cpp
    mapper.h
)

PEERDIR(
    kernel/doom/blob_cache
    kernel/doom/chunked_wad/protos
    kernel/doom/doc_lump_fetcher
    kernel/doom/wad
)

END()

