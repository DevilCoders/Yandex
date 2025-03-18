GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

DATA(arcadia/build/rules/go/vendor.policy)

REQUIREMENTS(network:full)

SIZE(MEDIUM)

SRCS(
    gomod.go
    modules_txt.go
    packages.go
    policy.go
    used_packages.go
    vendor.go
)

GO_TEST_SRCS(
    modules_txt_test.go
    packages_test.go
    policy_test.go
    vendor_test.go
)

END()

RECURSE(gotest)
