PY2TEST()

OWNER(g:cloud-billing)

SIZE(MEDIUM)
PEERDIR(
    contrib/python/mock
    cloud/billing/utils/scripts/apply_query_to_table
    yt/python/client
    yql/library/python
)
DEPENDS(ydb/library/yql/udfs/common/yson2)
INCLUDE(${ARCADIA_ROOT}/yql/library/local/ya.make.19_4.inc)
TEST_SRCS(
    test_apply_query_to_table.py
)

END()
