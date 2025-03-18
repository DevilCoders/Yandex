GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(
    fix.go
    packages.go
    policy.go
    sort.go
    yamake.go
)

GO_TEST_SRCS(
    fix_test.go
    packages_test.go
    sort_test.go
    yamake_test.go
)

END()

RECURSE(gotest)
