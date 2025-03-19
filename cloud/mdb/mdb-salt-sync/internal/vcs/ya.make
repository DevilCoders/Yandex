GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    checkouts.go
    vcs.go
)

GO_TEST_SRCS(vcs_test.go)

END()

RECURSE(gotest)
