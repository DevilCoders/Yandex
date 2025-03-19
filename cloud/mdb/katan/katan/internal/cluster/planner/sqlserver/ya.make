GO_LIBRARY()

OWNER(g:mdb)

SRCS(planner.go)

GO_TEST_SRCS(planner_test.go)

END()

RECURSE(gotest)
