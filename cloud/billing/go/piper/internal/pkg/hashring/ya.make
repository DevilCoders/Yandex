GO_LIBRARY()

SRCS(
    compatible.go
    doc.go
    hash.go
    hashring.go
)

GO_TEST_SRCS(compatible_test.go)

END()

RECURSE(gotest)
