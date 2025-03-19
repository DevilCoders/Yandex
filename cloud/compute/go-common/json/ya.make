GO_LIBRARY()

OWNER(g:cloud-compute)

SRCS(json.go)

GO_TEST_SRCS(json_test.go)

END()

RECURSE(gotest)
