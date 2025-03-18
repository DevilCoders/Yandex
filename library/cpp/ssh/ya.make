OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    sign_agent.cpp
    sign_key.cpp
    ssh.cpp
)

PEERDIR(
    contrib/libs/libssh2
    library/cpp/openssl/init
    library/cpp/openssl/holders
)

END()

RECURSE(ut)
