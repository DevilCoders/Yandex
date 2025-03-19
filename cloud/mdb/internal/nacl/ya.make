GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    box.go
    key.go
)

GO_XTEST_SRCS(
    box_test.go
    key_test.go
)

END()

RECURSE(
    cmd
    gotest
)
