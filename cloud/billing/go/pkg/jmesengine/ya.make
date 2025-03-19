GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(
    doc.go
    functions.go
    interpreter.go
    util.go
)

GO_TEST_SRCS(
    compliance_test.go
    interpreter_test.go
    util_test.go
)

END()

RECURSE(gotest)
