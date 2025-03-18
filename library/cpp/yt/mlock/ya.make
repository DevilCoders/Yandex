LIBRARY()

OWNER(
    g:yt
    prime
)

IF (OS_LINUX AND NOT SANITIZER_TYPE)
    SRCS(mlock_linux.cpp)
ELSE()
    SRCS(mlock_other.cpp)
ENDIF()

END()

IF (OS_LINUX AND NOT SANITIZER_TYPE)
    RECURSE(unittests)
ENDIF()