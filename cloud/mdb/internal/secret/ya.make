GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    mask.go
    string.go
)

GO_TEST_SRCS(mask_test.go)

GO_XTEST_SRCS(string_test.go)

END()

RECURSE(gotest)
