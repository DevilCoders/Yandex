GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backup-worker.go
    k8s.go
    worker.go
)

END()
