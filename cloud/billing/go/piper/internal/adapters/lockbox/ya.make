GO_LIBRARY()

SRCS(configurator.go)

GO_TEST_SRCS(
    configurator_test.go
    mock_LockboxClient_test.go
)

END()

RECURSE(gotest)
