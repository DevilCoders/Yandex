GO_PROGRAM(kms_e2e)

OWNER(
    g:cloud-marketplace
)

SRCS(
    kms_e2e.go
)

GO_TEST_SRCS(
    kms_e2e_test.go
)

END()

RECURSE(gotest)
