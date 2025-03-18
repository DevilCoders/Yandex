LIBRARY()

OWNER(
    moskupols
    galtsev
    g:contrib
    g:yabs-rt
    g:antiinfra
)

IF (OS_LINUX AND NOT SANITIZER_TYPE)
    PEERDIR(
        contrib/libs/luajit_21
    )
    CFLAGS(GLOBAL -DNO_SANITIZERS_SO_USE_LUAJIT)
ELSE()
    PEERDIR(
        contrib/libs/lua
    )
ENDIF()

SRCS(
    lua.cpp
)

END()
