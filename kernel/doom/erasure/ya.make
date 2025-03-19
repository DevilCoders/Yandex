LIBRARY()

OWNER(
    g:base
    ssmike
)

SRCS(
    wad_writer.cpp
)

PEERDIR(
    library/cpp/erasure
    library/cpp/offroad/codec
    kernel/doom/wad
    kernel/doom/flat_blob_storage
    kernel/doom/blob_storage
    kernel/doom/chunked_wad
    kernel/doom/offroad_common
    kernel/doom/standard_models_storage
)

END()
