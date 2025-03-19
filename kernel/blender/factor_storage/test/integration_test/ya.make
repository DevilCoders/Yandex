OWNER(
    g:blender
)

IF (OS_LINUX AND NOT MUSL AND NOT SANITIZER_TYPE)
    PY2TEST()

    SIZE(MEDIUM)

    TIMEOUT(600)

    DEPENDS(
        kernel/blender/factor_storage/test/integration_test/cpp_program
        kernel/blender/factor_storage/test/integration_test/python_program/py
        kernel/blender/factor_storage/test/integration_test/python_program/py3
        yql/udfs/blender/factor_storage
        ydb/library/yql/udfs/common/yson2
    )

    PEERDIR(
        kernel/blender/factor_storage/test/common
        yql/library/python
    )

    INCLUDE(${ARCADIA_ROOT}/yql/library/local/ya.make.19_4.inc)

    SRCDIR(kernel/blender/factor_storage/test)

    TEST_SRCS(test.py)

    END()
ENDIF()

RECURSE_FOR_TESTS(
    cpp_program
    python_program
)
