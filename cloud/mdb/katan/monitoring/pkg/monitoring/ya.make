GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    monitoring.go
    schedules.go
    zombie.go
)

GO_XTEST_SRCS(schedules_test.go)

END()

RECURSE(gotest)
