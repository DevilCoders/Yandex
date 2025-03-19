GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(jobs.go)

END()

RECURSE(
    config
    gotest
    internal
)
