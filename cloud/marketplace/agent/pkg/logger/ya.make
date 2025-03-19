GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(logger.go)

GO_TEST_SRCS(logger_test.go)

END()

RECURSE(gotest)
