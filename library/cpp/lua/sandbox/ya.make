LIBRARY()

OWNER(lucius)

PEERDIR(
    library/cpp/lua
    library/cpp/resource
)

SRCS(
    sandbox.cpp
)

RESOURCE(
    sandbox.lua lua_sandbox
)

END()
