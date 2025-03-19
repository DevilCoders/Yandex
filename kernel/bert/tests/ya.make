IF(OS_LINUX AND NOT MUSL)
    RECURSE(
        batch_processor
        batch_runner
        batch_runner/diff_tool
    )
ENDIF()
