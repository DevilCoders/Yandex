LIBRARY()

OWNER(pg)

#shut up, clang
NO_COMPILER_WARNINGS()

IF (OS_ANDROID OR OS_CYGWIN OR OS_WINDOWS)
    #avoid infinite recursion
ELSE()
    SRCS(
        gettimeofday.cpp
    )
ENDIF()

END()
