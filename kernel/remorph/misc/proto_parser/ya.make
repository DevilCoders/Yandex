LIBRARY()

OWNER(g:remorph)

SRCS(
    proto_parser.cpp
)

PEERDIR(
    contrib/libs/protobuf
    kernel/gazetteer/protoparser
)

END()
