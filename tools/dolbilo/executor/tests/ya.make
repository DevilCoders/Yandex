OWNER(
    g:base
    mvel
    darkk
)

PY2TEST()

TEST_SRCS(test_smoke.py)

PEERDIR(tools/dolbilo/executor/tests/lib)

DATA(arcadia/tools/dolbilo/executor/tests)

DEPENDS(
    tools/dolbilo/planner
    tools/dolbilo/executor
    balancer/daemons/balancer
)



END()

RECURSE(long)
