OWNER(g:cloud-nbs)

GO_PROGRAM()

SRCS(
    auth.go
    main.go
    cmd_disks.go
    cmd_filesystem.go
    cmd_images.go
    cmd_operations.go
    cmd_placement_group.go
    cmd_private.go
    cmd_snapshots.go
    cmd_tasks.go
    common.go
)

END()
