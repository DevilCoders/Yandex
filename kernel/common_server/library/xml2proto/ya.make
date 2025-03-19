LIBRARY()
OWNER(g:cs_dev)

PEERDIR(
    library/cpp/xml/document
    contrib/libs/protobuf
    library/cpp/protobuf/util
    library/cpp/logger/global
)
SRCS(
    converter.cpp
)

END()

RECURSE_FOR_TESTS(ut)
