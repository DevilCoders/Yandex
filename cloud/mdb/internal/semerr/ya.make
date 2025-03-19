GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    doc.go
    semantic.go
)

GO_TEST_SRCS(
    semantic_errorf_formatting_test.go
    semantic_multiwrap_formatting_test.go
    semantic_new_formatting_test.go
    semantic_test.go
)

END()

RECURSE(gotest)
