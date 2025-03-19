OWNER(g:cloud-dwh)

IF(OS_LINUX AND CLANG AND NOT SANITIZER_TYPE AND NOT WITH_VALGRIND)
    PY3TEST()

    SIZE(MEDIUM)

    PEERDIR(
        cloud/dwh/test_utils/yt
        library/python/resource
    )

    INCLUDE(${ARCADIA_ROOT}/yql/library/local/ya.make.19_4.inc)

    DEPENDS(
        ydb/library/yql/udfs/common/datetime2
        ydb/library/yql/udfs/common/digest
        ydb/library/yql/udfs/common/string
        ydb/library/yql/udfs/common/yson2
    )

    RESOURCE(
        cloud/dwh/yql/utils/datetime.sql yql/utils/datetime.sql
        cloud/dwh/yql/utils/helpers.sql yql/utils/helpers.sql
        cloud/dwh/yql/utils/numbers.sql yql/utils/numbers.sql
        cloud/dwh/yql/utils/tables.sql yql/utils/tables.sql
        cloud/dwh/yql/utils/currency.sql yql/utils/currency.sql
    )

    TEST_SRCS(
        test_datetime.py
        test_helpers.py
        test_numbers.py
        test_tables.py
        test_currency.py
    )

    END()
ENDIF()
