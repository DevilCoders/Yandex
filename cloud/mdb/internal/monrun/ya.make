GO_LIBRARY()

OWNER(g:mdb)

SRCS(monrun.go)

GO_XTEST_SRCS(monrun_test.go)

END()

RECURSE(
    gotest
    runner
)
