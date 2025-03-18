OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    sslsock.cpp
)

PEERDIR(
    library/cpp/http/fetch
    library/cpp/openssl/init
    library/cpp/openssl/method
)

END()
