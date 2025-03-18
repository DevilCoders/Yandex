LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    library/cpp/auth_client_parser
)

SRCS(
    cookie.cpp
    token.cpp
)

END()
