PROGRAM(as-client)

OWNER(g:cloud-iam)

SRCS(
    main.cpp
    ../ut/mock-server.cpp
)

CFLAGS(-DARCADIA_BUILD)

PEERDIR(
    cloud/iam/accessservice/client/iam-access-service-client-cpp/1.0.0/src
)

END()
