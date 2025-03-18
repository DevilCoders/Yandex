LIBRARY()

OWNER(g:contrib)

BUILD_ONLY_IF(OS_WINDOWS)

SRCS(
    srvman.cpp
    winservice.cpp
)

END()
