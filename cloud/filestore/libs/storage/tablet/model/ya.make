LIBRARY()

OWNER(g:cloud-nbs)

GENERATE_ENUM_SERIALIZATION(alloc.h)

SRCS(
    alloc.cpp
    binary_reader.cpp
    binary_writer.cpp
    blob.cpp
    blob_builder.cpp
    block.cpp
    block_buffer.cpp
    block_list.cpp
    block_list_decode.cpp
    block_list_encode.cpp
    block_list_spec.cpp
    channels.cpp
    compaction_map.cpp
    deletion_markers.cpp
    fresh_blocks.cpp
    fresh_bytes.cpp
    garbage_queue.cpp
    group_by.cpp
    mixed_blocks.cpp
    operation.cpp
    range.cpp
    range_locks.cpp
    split_range.cpp
)

PEERDIR(
    cloud/filestore/libs/storage/model
    cloud/storage/core/libs/common
    cloud/storage/core/libs/tablet/model
    library/cpp/containers/intrusive_rb_tree
    library/cpp/containers/stack_vector
)

END()

RECURSE_FOR_TESTS(ut)
