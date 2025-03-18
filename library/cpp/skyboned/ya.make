LIBRARY()

OWNER(
    vtyulb
    g:saas2
)

SRCS(
    api.cpp
    manager.cpp
    share_object.cpp
    sky_share.cpp
)

PEERDIR(
    contrib/libs/bencode
    library/cpp/http/client
    library/cpp/openssl/crypto
    library/cpp/tvmauth/client
    library/cpp/threading/future
)

END()

RECURSE(
    example
    ut
)
