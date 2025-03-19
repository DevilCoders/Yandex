GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    auth.go
    blackboxauth.go
    cache.go
    login.go
    scope.go
    service.go
)

GO_TEST_SRCS(
    cache_test.go
    login_test.go
    scope_test.go
    service_test.go
)

END()

RECURSE(gotest)
