GO_LIBRARY()

OWNER(g:cloud-ps)

SRCS(calendar.go)

GO_TEST_SRCS(calendar_test.go)

END()

RECURSE(gotest)
