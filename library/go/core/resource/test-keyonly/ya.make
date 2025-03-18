GO_TEST(test)

OWNER(
    prime
    g:go-library
)

RESOURCE(
    - foo=bar
    - bar=baz
)

GO_TEST_SRCS(resource_test.go)

END()
