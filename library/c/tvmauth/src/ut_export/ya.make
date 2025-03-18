PY2TEST()

OWNER(g:passport_infra)

TEST_SRCS(test.py)

DEPENDS(library/c/tvmauth/so)

DATA(arcadia/library/c/tvmauth)

IF (OS_LINUX)
    # ubuntu 14
    DECLARE_EXTERNAL_RESOURCE(
        SYSROOT_FOR_TEST
        sbr:243881007
    )
    # clang 5
    DECLARE_EXTERNAL_RESOURCE(
        COMPILER_FOR_TEST
        sbr:526275170
    )
    DECLARE_EXTERNAL_RESOURCE(
        LD_FOR_TEST
        sbr:360800403
    )
ENDIF()



END()
