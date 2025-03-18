LIBRARY()

OWNER(g:bsyeti)

SRCS(
    msgpack2json.cpp
)

PEERDIR(
    contrib/libs/msgpack
    library/cpp/json
)

END()
