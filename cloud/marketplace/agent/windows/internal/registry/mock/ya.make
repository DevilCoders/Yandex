GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(registry_mock.go)

GO_TEST_SRCS(registry_mock_test.go)

END()

RECURSE(gotest)
