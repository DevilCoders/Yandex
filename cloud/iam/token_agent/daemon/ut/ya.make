GTEST()

OWNER(g:cloud-iam)

SIZE(SMALL)

SRCS(
    config_ut.cpp
    http_token_service_ut.cpp
    iam_token_client_ut.cpp
    iam_token_client_utils_ut.cpp
    group_ut.cpp
    mock/mock_iam_token_service.cpp
    mock/mock_tpm_agent_service.cpp
    role_ut.cpp
    server_ut.cpp
    soft_tpm_ut.cpp
    updater_ut.cpp
    user_ut.cpp
)

PEERDIR(
    ADDINCL cloud/iam/token_agent/daemon/lib

    contrib/libs/curl
    contrib/libs/jwt-cpp
    library/cpp/json
)

END()

