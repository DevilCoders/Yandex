GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    register.go
    roll.go
)

GO_TEST_SRCS(
    register_test.go
    roll_test.go
)

END()

RECURSE(
    gotest
    health
    locker
    planner
)
