LIBRARY()

OWNER(
    g:base
    sankear
)

SRCS(
    chunked_blob_storage.h
    chunked_blob_storage_with_cache.h
    direct_aio_wad_chunked_blob_storage.h
    direct_io_wad_chunk.h
    mapped_wad_chunk.h
    mapped_wad_chunked_blob_storage.h
    wad_chunk.h
    wad_chunked_blob_storage.h
)

PEERDIR(
    kernel/doom/direct_io
    library/cpp/offroad/custom
    library/cpp/offroad/flat
    library/cpp/histogram/simple
    library/cpp/containers/stack_vector
    library/cpp/threading/thread_local
)

END()
