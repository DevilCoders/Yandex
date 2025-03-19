GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    helpers.go
    maintainer.go
)

GO_TEST_SRCS(
    helpers_test.go
    maintainer_test.go
)

END()

RECURSE(gotest)
