GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    struct_mapper.go
)

GO_TEST_SRCS(
    struct_mapper_test.go
)

END()

RECURSE(
    gotest
)
