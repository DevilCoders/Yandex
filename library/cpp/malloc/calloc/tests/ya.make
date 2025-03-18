IF (SANITIZER_TYPE)
    # no custom allocators with sanitizer
ELSE()
    RECURSE(
        do_with_enabled
        do_with_disabled
        py_runner
    )
ENDIF()
