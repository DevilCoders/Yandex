UNITTEST_FOR(cloud/blockstore/libs/storage/partition/model)

OWNER(g:cloud-nbs)

SRCS(
    barrier_ut.cpp
    block_index_ut.cpp
    block_mask_ut.cpp
    checkpoint_ut.cpp
    cleanup_queue_ut.cpp
    commit_queue_ut.cpp
    fresh_blob_ut.cpp
    garbage_queue_ut.cpp
    mixed_index_cache_ut.cpp
)

PEERDIR(
    library/cpp/resource
)

RESOURCE(
    data/fresh_write.blob fresh_write.blob
    data/fresh_zero.blob fresh_zero.blob
)

END()
