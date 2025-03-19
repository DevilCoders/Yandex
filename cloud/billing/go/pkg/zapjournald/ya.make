GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(
    core.go
    encode.go
)

GO_TEST_SRCS(
    core_test.go
    encode_test.go
)

END()

RECURSE(gotest)
