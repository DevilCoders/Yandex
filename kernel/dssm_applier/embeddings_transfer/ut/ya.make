IF(OS_LINUX AND NOT MUSL)
    RECURSE(
        test
        diff_tool
        applier
    )
ENDIF()
