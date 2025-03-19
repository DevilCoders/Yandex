GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    accelerator.go
    roll.go
    roller.go
)

GO_XTEST_SRCS(
    accelerator_test.go
    roll_test.go
    roller_test.go
)

END()

RECURSE(gotest)
