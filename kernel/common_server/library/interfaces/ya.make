LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/protobuf/json
    library/cpp/json
    library/cpp/tvmauth/client
    contrib/libs/protobuf
)

SRCS(
    proto.cpp
    container.cpp
    tvm_manager.cpp
)

END()
