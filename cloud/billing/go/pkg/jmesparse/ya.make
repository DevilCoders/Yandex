GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(
    astnodetype_string.go
    doc.go
    lexer.go
    parser.go
    toktype_string.go
)

GO_TEST_SRCS(
    lexer_test.go
    parser_test.go
)

END()

RECURSE(gotest)
