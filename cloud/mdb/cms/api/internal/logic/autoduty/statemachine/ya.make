GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    base.go
    input.go
    side_effects.go
    state_machine.go
)

GO_XTEST_SRCS(state_machine_test.go)

END()

RECURSE(gotest)
