GO_LIBRARY()

OWNER(g:cloud-iam)

SRCS(iam_mock.go)

GO_TEST_SRCS(iam_mock_test.go)

END()

RECURSE(gotest)
