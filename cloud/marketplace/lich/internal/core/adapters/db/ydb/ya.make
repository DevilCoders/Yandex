GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    product_versions.go
    transforms.go
)

GO_TEST_SRCS(
    transforms_test.go
)

END()


RECURSE(
    gotest
)
