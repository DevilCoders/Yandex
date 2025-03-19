GO_LIBRARY()

SRCS(
    common.go
    dump.go
    parse.go
    resharder.go
    ydbpresenter.go
)

GO_TEST_SRCS(
    common_test.go
    dump_test.go
    generate_test.go
    parse_test.go
    resharder_test.go
    ydbpresenter_test.go
)

END()

RECURSE(
    gotest
    mocks
)
