UNITTEST_FOR(cloud/filestore/libs/server)

OWNER(g:cloud-nbs)

SRCS(
    server_ut.cpp
)

PEERDIR(
    cloud/filestore/libs/client
    cloud/filestore/libs/diagnostics
)

END()
