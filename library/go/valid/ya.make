GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(
    compare.go
    context.go
    credit_card.go
    data_url.go
    doc.go
    errors.go
    isbn.go
    luhn.go
    semver.go
    string.go
    struct.go
    uuid.go
    validator.go
)

GO_XTEST_SRCS(
    compare_test.go
    context_test.go
    credit_card_test.go
    data_url_test.go
    errors_test.go
    example_validation_test.go
    isbn_test.go
    luhn_test.go
    semver_test.go
    string_test.go
    struct_test.go
    uuid_test.go
)

END()

RECURSE(
    gotest
    v2
)
