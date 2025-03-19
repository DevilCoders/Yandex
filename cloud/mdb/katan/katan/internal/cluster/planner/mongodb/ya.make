GO_LIBRARY()

OWNER(g:mdb)

REQUIREMENTS(ram:12)

SRCS(planner.go)

GO_TEST_SRCS(planner_test.go)

END()

RECURSE(gotest)
