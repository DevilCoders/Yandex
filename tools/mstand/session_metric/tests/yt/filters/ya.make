PY3TEST()

OWNER(g:mstand)

TEST_SRCS(
    filters.py
)

SIZE(MEDIUM)

PEERDIR(
    yt/python/client
    mapreduce/yt/python
    tools/mstand/session_metric/tests/yt/test_runner
    tools/mstand/session_metric/tests/yt/test_metrics
)

DEPENDS(
    yt/packages/latest
)

DATA(
    arcadia/tools/mstand/session_metric/tests/yt/filters/data/
    #local_cypress_tree
    sbr://1133254170
)

END()
