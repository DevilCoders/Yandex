RECURSE(
    canonizer
    canonizer/ut
    pycanonizer
    pycanonizer/ut
)

IF (NOT OS_WINDOWS)
    RECURSE(
        login
    )
ENDIF()
