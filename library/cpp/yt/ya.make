RECURSE(
    assert
    coding
    exception
    misc
    string
    system
    stockpile
    yson
    yson_string
)

IF (NOT OS_WINDOWS)
    RECURSE(
        containers
        cpu_clock
        farmhash
        logging
        malloc
        memory
        mlock
        phdr_cache
        small_containers
        threading
    )
ENDIF()
