UNITTEST_FOR(cloud/filestore/libs/storage/tablet/model)

OWNER(g:cloud-nbs)

SRCS(
    block_buffer_ut.cpp
    block_list_ut.cpp
    channels_ut.cpp
    compaction_map_ut.cpp
    deletion_markers_ut.cpp
    fresh_blocks_ut.cpp
    fresh_bytes_ut.cpp
    garbage_queue_ut.cpp
    mixed_blocks_ut.cpp
    operation_ut.cpp
    range_locks_ut.cpp
    range_ut.cpp
    split_range_ut.cpp
)

END()
