PY2MODULE(pytestproto PREFIX lib)

OWNER(
    ermolovd
    g:messagebus
)

PEERDIR(
    library/python/messagebus/wrapper
    library/cpp/messagebus/protobuf
)

SRCS(
    testmessage.proto
    testproto.swg
    wrapper.cpp
)

END()

RECURSE_ROOT_RELATIVE(
    contrib/libs/protobuf/python
)
