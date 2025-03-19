GTEST()

OWNER(g:cloud-iam)

SIZE(SMALL)

SRCS(
    mock-server.cpp
    authenticate.cpp
    authorize.cpp
)

CFLAGS(-DARCADIA_BUILD)

PEERDIR(
    cloud/iam/accessservice/client/iam-access-service-client-cpp/1.0.0/src
)

END()

