GO_LIBRARY()

OWNER(g:mdb)

SRCS(planner.go)

GO_XTEST_SRCS(planner_test.go)

END()

RECURSE(gotest)
