GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(billing.go)

GO_TEST_SRCS(billing_test.go)

END()

RECURSE(gotest)
