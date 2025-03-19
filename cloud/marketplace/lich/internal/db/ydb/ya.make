GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    product_versions.go
)

GO_TEST_SRCS(
    base_test.go
    product_versions_test.go
)

END()

RECURSE(gotest)
