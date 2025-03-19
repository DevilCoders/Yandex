PY2TEST()

OWNER(g:cloud-billing)

SIZE(SMALL)

PEERDIR(
    contrib/python/mock
    cloud/billing/utils/scripts/upload_test_data_to_yt
    yt/python/client
)
INCLUDE(${ARCADIA_ROOT}/mapreduce/yt/python/recipe/recipe.inc)
TEST_SRCS(
    test_upload_test_data_to_yt.py
)

END()
