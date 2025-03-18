GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

REQUIREMENTS(network:full)

SRCS(
    main_module.go
    module.go
    package.go
    toolchain.go
)

GO_XTEST_SRCS(main_module_test.go)

END()

RECURSE(gotest)
