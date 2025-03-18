GTEST()

OWNER(bulatman g:cpp-contrib)

SRCS(
    data.proto

    gtest_protobuf_ut.cpp
)

PEERDIR(
    library/cpp/testing/gtest_protobuf
)

END()
