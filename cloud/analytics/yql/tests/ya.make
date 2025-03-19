PY2TEST()
# Why not py3? Because PEERDIR from module tagged PY3 to yql/library/fastcheck/python is prohibited

OWNER(g:cloud_analytics)

PEERDIR(
    yql/library/fastcheck/python
)

DATA(
    arcadia/cloud/analytics/yql
)

TEST_SRCS(
    test_yql_fast.py
)

END()

