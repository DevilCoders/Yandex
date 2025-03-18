OWNER(g:ci)

PROTO_LIBRARY()

GRPC()

PEERDIR(
    ci/proto
)

SRCS(
    actions.proto
    aggregate_statistics.proto
    check.proto
    check_iteration.proto
    check_task.proto
    common.proto
    events_stream_messages.proto
    main_stream_messages.proto
    shard_in.proto
    shard_out.proto
    storage_api.proto
    storage_public_api.proto
    storage_front_api.proto
    storage_front_history_api.proto
    storage_front_tests_api.proto
    storage_proxy_api.proto
    task_messages.proto
    post_processor.proto
    dovecote.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
