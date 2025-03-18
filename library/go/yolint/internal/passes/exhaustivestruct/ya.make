GO_LIBRARY()

OWNER(
    g:strm-admins
    grihabor
)

SRCS(exhaustivestruct.go)

GO_XTEST_SRCS(exhaustivestruct_test.go)

END()

RECURSE(gotest)
