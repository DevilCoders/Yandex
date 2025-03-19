PY2TEST()

OWNER(g:cloud-billing)

SIZE(SMALL)

PEERDIR(
    contrib/python/mock
    cloud/billing/utils/scripts/enrich_sales_name_table
    yt/python/client
)
INCLUDE(${ARCADIA_ROOT}/mapreduce/yt/python/recipe/recipe.inc)
TEST_SRCS(
    test_enrich_sales_name_table.py
)

END()
