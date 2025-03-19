GO_LIBRARY()

SRCS(saltformula_controller.go)

GO_TEST_SRCS(
    saltformula_controller_test.go
    suite_test.go
)

END()

RECURSE(gotest)
