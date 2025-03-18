OWNER(
    divankov
    g:snippets
)

IF (NOT OS_WINDOWS)
    PY2TEST()

    TEST_SRCS(urlcut_test.py)

    DATA(arcadia_tests_data/snippets_tests_data)

    DEPENDS(tools/snipmake/urlcut_test)



END()

ENDIF()
