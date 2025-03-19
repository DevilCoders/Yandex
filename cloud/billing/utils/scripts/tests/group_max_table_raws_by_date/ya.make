PY2TEST()

OWNER(g:cloud-billing)

SIZE(MEDIUM)
PEERDIR(
    contrib/python/mock
    cloud/billing/utils/scripts/group_max_table_raws_by_date
    yt/python/client
    yql/library/python
)
INCLUDE(${ARCADIA_ROOT}/yql/library/local/ya.make.19_4.inc)
TEST_SRCS(
    test_group_max_table_raws_by_date.py
)

END()
