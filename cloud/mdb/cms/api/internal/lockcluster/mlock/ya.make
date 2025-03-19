GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    acquire.go
    common.go
    release.go
    reuse.go
)

GO_XTEST_SRCS(
    acquire_test.go
    release_test.go
)

END()

RECURSE(gotest)
