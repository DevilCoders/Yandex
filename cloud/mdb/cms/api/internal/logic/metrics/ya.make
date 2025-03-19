GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    autoduty.go
    base.go
    gather.go
)

GO_TEST_SRCS(autoduty_test.go)

END()

RECURSE(gotest)
