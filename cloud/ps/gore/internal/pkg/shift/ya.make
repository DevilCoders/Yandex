GO_LIBRARY()

OWNER(g:cloud-ps)

SRCS(shift.go)

GO_TEST_SRCS(shift_test.go)

END()

RECURSE(gotest)
