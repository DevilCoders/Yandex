GO_LIBRARY()

SRCS(
    config.go
    merge.go
)

GO_TEST_SRCS(
    config_test.go
    merge_test.go
)

END()

RECURSE(gotest)
