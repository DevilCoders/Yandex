IF (NOT OS_WINDOWS)
    RECURSE(
        ut_impl
        force_run_avx2
        force_run_sse
    )
ENDIF()

