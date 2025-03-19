LIBRARY()

OWNER(
    sskvor
    g:base
)

SRCS(
    blob_storage_cache.cpp
    consistent_lumps_mapping.cpp
    item_storage.cpp
    lumps_set.cpp
    request.cpp
    status.cpp
    types.cpp
    wad_item_fetcher.cpp
    wad_item_storage.cpp
    wad_item_storage_writer.cpp
)

PEERDIR(
    kernel/doom/blob_storage
    kernel/doom/blob_cache
    kernel/doom/chunked_wad
    kernel/doom/info
    kernel/doom/item_storage/proto
    kernel/doom/offroad_minhash_wad
    kernel/doom/wad
    library/cpp/containers/absl_flat_hash
    library/cpp/containers/stack_vector
    library/cpp/int128
    library/cpp/iterator
    library/cpp/offroad/custom
)

END()

RECURSE_FOR_TESTS(
    ut
)
