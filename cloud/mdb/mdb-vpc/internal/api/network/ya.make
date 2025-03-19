GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    create.go
    delete.go
    get.go
    import.go
    list.go
    server.go
)

END()

RECURSE(gotest)
