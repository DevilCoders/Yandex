PY23_TEST()

OWNER(g:cloud-nbs)

SIZE(MEDIUM)

TEST_SRCS(
    test.py
)

PEERDIR(
    cloud/blockstore/tools/analytics/filter-yt-logs/lib
)

INCLUDE(${ARCADIA_ROOT}/mapreduce/yt/python/recipe/recipe.inc)

END()
