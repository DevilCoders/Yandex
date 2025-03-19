GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    create.go
    update.go
)

GO_TEST_SRCS(create_test.go)

END()

RECURSE(gotest)
