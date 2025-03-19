GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clean.go
    cmds.go
    update.go
    wakeup.go
)

GO_TEST_SRCS(update_test.go)

END()

RECURSE(gotest)
