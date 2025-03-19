OWNER(
    dmitryno
    gotmanov
    g:wizard
)

LIBRARY()

PEERDIR(
    contrib/libs/protobuf
    contrib/libs/protoc
    library/cpp/charset
    library/cpp/containers/compact_vector
    library/cpp/deprecated/iter
    library/cpp/digest/md5
    library/cpp/protobuf/json
    library/cpp/protobuf/util
    library/cpp/deprecated/atomic
)

SRCS(
    tools.cpp
    protohelpers.cpp
    binaryguard.cpp
    parserbase.cpp
    hashindex.cpp
)

END()
