GO_LIBRARY()

OWNER(
    gzuykov
    g:go-library
)

SRCS(importcheck.go)

GO_XTEST_SRCS(importcheck_test.go)

END()

RECURSE(gotest)
