UNITTEST()

OWNER(
    kostik
    g:orange
)

PEERDIR(
    ADDINCL library/cpp/protobuf/protofile
)

SRCDIR(library/cpp/protobuf/protofile)

SRCS(
    protofile_ut.cpp
    protofile_ut.proto
)

END()
