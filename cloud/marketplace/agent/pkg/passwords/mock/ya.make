GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(passwords_mock.go)

GO_TEST_SRCS(passwords_mock_test.go)

END()

RECURSE(gotest)
