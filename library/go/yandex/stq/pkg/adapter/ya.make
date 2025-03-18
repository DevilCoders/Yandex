GO_LIBRARY()

OWNER(ihelos)

SRCS(adapter.go)

GO_TEST_SRCS(adapter_test.go)

END()

RECURSE(gotest)
