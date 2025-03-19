GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    create.go
    get.go
    release.go
)

GO_TEST_SRCS(
    create_test.go
    get_test.go
    release_test.go
)

END()

RECURSE(gotest)
