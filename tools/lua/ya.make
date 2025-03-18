PROGRAM()

OWNER(pg)

NO_COMPILER_WARNINGS()

PEERDIR(
    contrib/libs/lua
)

SRCDIR(contrib/libs/lua/lua-5.2.0/src)

ADDINCL(contrib/libs/lua/lua-5.2.0/src)

SRCS(
    lua.cpp
)

SET(IDE_FOLDER "_Builders")

END()
