GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    cms.go
)

GO_TEST_SRCS(
    cms_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
