GO_LIBRARY()

SRCS(
    adapter.go
    common.go
    folder.go
)

GO_TEST_SRCS(
    adapter_test.go
    common_test.go
    folder_test.go
)

END()

RECURSE(gotest)
