OWNER(g:cpp-contrib)

RECURSE(
    cyson
    tvmauth
    tvmauth/so
)

IF (NOT OS_WINDOWS)
    RECURSE(
        tvmauth/src/ut
    )
    IF (NOT SANITIZER_TYPE)
        RECURSE(
            tvmauth/src/ut_export
        )
    ENDIF()
ENDIF()
