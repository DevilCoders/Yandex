GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(
    each.go
    empty.go
    errors.go
    kind.go
    length.go
    luhn.go
    map.go
    message.go
    path.go
    range.go
    regex.go
    required.go
    rule.go
    semver.go
    slice.go
    string.go
    time.go
    unique.go
    url.go
    uuid.go
)

GO_TEST_SRCS(
    each_test.go
    empty_test.go
    errors_test.go
    kind_test.go
    length_test.go
    luhn_test.go
    map_test.go
    message_test.go
    path_test.go
    range_test.go
    regex_test.go
    required_test.go
    semver_test.go
    slice_test.go
    string_test.go
    time_test.go
    unique_test.go
    uuid_test.go
)

END()

RECURSE(gotest)
