OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    collect_snapshots_task.go
    consts.go
    create_snapshot_from_disk_task.go
    create_snapshot_from_legacy_snapshot_task.go
    create_snapshot_from_snapshot_task.go
    create_snapshot_from_url_task.go
    delete_snapshot_data_task.go
    delete_snapshot_task.go
    register.go
    transfer_from_disk_to_disk_task.go
    transfer_from_snapshot_to_disk_task.go
)

END()

RECURSE(
    common
    config
    nbs
    protos
    snapshot
    test
)

RECURSE_FOR_TESTS(
    transfer_tests
)
