GO_LIBRARY()

OWNER(
    buglloc
    g:passport_infra
    g:go-library
)

SRCS(cache.go)

GO_XTEST_SRCS(cache_test.go)

END()

RECURSE_FOR_TESTS(gotest)
