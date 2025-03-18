LIBRARY()

OWNER(g:aapi)

PEERDIR(
    aapi/lib/node
    contrib/libs/openssl
)

SRCS(
    git_hash.cpp
    walk.cpp
)

END()
