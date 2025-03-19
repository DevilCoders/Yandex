PY3TEST()

OWNER(g:cloud-nbs)

SIZE(MEDIUM)

TEST_SRCS(
    test.py
)

PEERDIR(
    cloud/blockstore/tools/analytics/find-perf-bottlenecks/lib
    cloud/blockstore/tools/analytics/find-perf-bottlenecks/ytlib
)

DATA(
    arcadia/cloud/blockstore/tools/analytics/find-perf-bottlenecks/tests/data
)

INCLUDE(${ARCADIA_ROOT}/mapreduce/yt/python/recipe/recipe.inc)

END()
