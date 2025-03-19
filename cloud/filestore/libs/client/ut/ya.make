UNITTEST_FOR(cloud/filestore/libs/client)

OWNER(g:cloud-nbs)

SRCS(
    client_ut.cpp
    durable_ut.cpp
    session_ut.cpp
)

PEERDIR(
    cloud/filestore/libs/diagnostics
    cloud/filestore/libs/server
)

END()
