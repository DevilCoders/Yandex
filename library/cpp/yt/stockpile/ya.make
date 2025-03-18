LIBRARY()

OWNER(
    g:yt
    prime
)

IF (OS_LINUX AND NOT SANITIZER_TYPE)
    SRCS(stockpile_linux.cpp)
ELSE()
    SRCS(stockpile_other.cpp)
ENDIF()

END()
