OWNER(g:cloud-billing)

IF(OS_LINUX AND CLANG AND NOT SANITIZER_TYPE AND NOT WITH_VALGRIND)
    PY3TEST()
    SIZE(MEDIUM)
    PEERDIR(
        cloud/dwh/test_utils/yt
        cloud/billing/utils/scripts/aggregate_balance_reports
        yt/python/client
        yql/library/python
    )
    DEPENDS(
        ydb/library/yql/udfs/common/datetime2
    )
    INCLUDE(${ARCADIA_ROOT}/yql/library/local/ya.make.19_4.inc)
    TEST_SRCS(
        test_aggregate_balance_reports.py
    )

    END()
ENDIF()
