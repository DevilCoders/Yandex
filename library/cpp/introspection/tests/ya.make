LIBRARY()

OWNER(
    g:util
    emazhukin
)

SRCS(data.cpp)

END()

IF (NOT OS_WINDOWS)
    RECURSE_FOR_TESTS(benchmark)
ENDIF()

RECURSE_FOR_TESTS(
    ut
)
