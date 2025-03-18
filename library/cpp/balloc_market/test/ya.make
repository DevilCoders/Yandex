IF (NOT SANITIZER_TYPE)
    # no custom allocators with sanitizer
    RECURSE(
        do_with_enabled
        do_with_disabled
    )
ENDIF()
