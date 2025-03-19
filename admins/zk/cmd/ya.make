GO_PROGRAM(zk)

OWNER(
    skacheev
    g:music-sre
)

SRCS(
    children.go
    create.go
    delete.go
    dump.go
    exists.go
    get.go
    helpers.go
    main.go
    restore.go
    set.go
    stat.go
)

GO_TEST_SRCS(restore_test.go)

END()

RECURSE(gotest)
