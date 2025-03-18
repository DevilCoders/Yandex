UNITTEST_FOR(library/c/tvmauth)

OWNER(g:passport_infra)

DATA(arcadia/library/cpp/tvmauth/client/ut/files)

SRCS(
    c_interface_ut.cpp
    high_lvl_client_ut.cpp
    high_lvl_wrapper_ut.cpp
    utils_ut.cpp
    wrapper_ut.cpp
)

PEERDIR(
    library/cpp/testing/mock_server
)

END()
