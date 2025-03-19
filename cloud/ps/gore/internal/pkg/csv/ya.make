GO_LIBRARY()

OWNER(g:cloud-ps)

SRCS(
    csv.go
    errors.go
)

GO_TEST_SRCS(csv_test.go)

END()

RECURSE(gotest)
