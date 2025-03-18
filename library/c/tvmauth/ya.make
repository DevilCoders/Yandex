LIBRARY(tvmauth)

OWNER(g:passport_infra)

PEERDIR(
    library/cpp/tvmauth/client
)

SRCS(
    deprecated.cpp
    high_lvl_client.cpp
    tvmauth.cpp
    src/c_validation.c
)

END()
