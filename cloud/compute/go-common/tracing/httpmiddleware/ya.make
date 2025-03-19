GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(httpmiddleware.go)

GO_TEST_SRCS(middleware_test.go)

END()

RECURSE(gotest)
