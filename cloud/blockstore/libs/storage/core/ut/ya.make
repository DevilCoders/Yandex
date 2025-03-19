UNITTEST_FOR(cloud/blockstore/libs/storage/core)

OWNER(g:cloud-nbs)

SRCS(
    block_handler_ut.cpp
    compaction_map_ut.cpp
    compaction_policy_ut.cpp
    metrics_ut.cpp
    mount_token_ut.cpp
    ring_buffer_ut.cpp
    volume_label_ut.cpp
    volume_model_ut.cpp
    write_buffer_request_ut.cpp
)

PEERDIR(
    cloud/blockstore/libs/storage/testlib
    cloud/storage/core/libs/tablet
)


   YQL_LAST_ABI_VERSION()


END()
