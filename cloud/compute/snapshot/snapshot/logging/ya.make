GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    logging.go
    names.go
)

GO_TEST_SRCS(logging_test.go)

END()

RECURSE(gotest)
