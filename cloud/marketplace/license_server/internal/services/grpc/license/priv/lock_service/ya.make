GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    create.go
    get.go
    get_locked_status.go
    locks.go
    release.go
)

END()
