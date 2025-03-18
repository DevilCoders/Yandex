OWNER(g:cpp-contrib)

UNITTEST()

SRCS(
    parse_ut.cpp
    pool_ut.cpp
)

IF (OS_LINUX)
    SRCS(client_ut.cpp)
ENDIF()


PEERDIR(
    library/cpp/http/client/fetch
    library/cpp/http/client
    library/cpp/http/server
)

REQUIREMENTS(ram:32)

END()
