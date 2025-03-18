UNITTEST()

OWNER(
    g:contrib
    g:cpp-contrib
)

NO_COMPILER_WARNINGS()

SRCS(
    messagext_ut.cpp
    test.proto
    ns.proto
)

PEERDIR(
    contrib/libs/protobuf
    contrib/libs/protoc
)

END()
