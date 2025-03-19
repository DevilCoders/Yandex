GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(
    api.go
    application.go
    final_status.go
    state.go
)

END()

RECURSE(
    http
    mocks
)
