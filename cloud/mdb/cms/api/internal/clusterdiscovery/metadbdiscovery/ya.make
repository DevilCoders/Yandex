GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    base.go
    closest.go
)

GO_TEST_SRCS(closest_test.go)

END()

RECURSE(gotest)
