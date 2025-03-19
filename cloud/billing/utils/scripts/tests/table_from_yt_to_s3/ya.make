PY2TEST()

OWNER(g:cloud-billing)

SIZE(SMALL)

PEERDIR(
    contrib/python/mock
    cloud/billing/utils/scripts/table_from_yt_to_s3
    yt/python/client
)
INCLUDE(${ARCADIA_ROOT}/mapreduce/yt/python/recipe/recipe.inc)
TEST_SRCS(
    test_table_ba_from_yt_to_s3.py
)

END()
