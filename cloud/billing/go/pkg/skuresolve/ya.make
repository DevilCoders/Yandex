GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(
    formula.go
    jmes.go
    json.go
    matcher.go
    rules.go
    types.go
)

GO_TEST_SRCS(
    formula_test.go
    jmes_test.go
    matcher_test.go
    rules_test.go
)

END()

RECURSE(gotest)
