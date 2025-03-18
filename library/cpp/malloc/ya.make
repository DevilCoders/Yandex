RECURSE(
    api
    tcmalloc
    galloc
    jemalloc
    lockless
    nalf
    sample-client
    system
    mimalloc
    hu
)

IF (NOT OS_WINDOWS)
    RECURSE(
        calloc
        calloc/tests
        calloc/calloc_profile_diff
        calloc/calloc_profile_scan
        calloc/calloc_profile_scan/ut
    )
ENDIF()
