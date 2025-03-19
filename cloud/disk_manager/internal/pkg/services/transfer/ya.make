OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    interface.go
    transfer_from_image_to_disk_task.go
    transfer_from_snapshot_to_disk_task.go
    register.go
    service.go
)

END()

RECURSE(
    config
    protos
)

RECURSE_FOR_TESTS(
    mocks
)
