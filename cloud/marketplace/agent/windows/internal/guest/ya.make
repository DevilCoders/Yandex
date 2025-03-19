GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(guest.go)

GO_TEST_SRCS(guest_test.go)

END()

RECURSE(gotest)
