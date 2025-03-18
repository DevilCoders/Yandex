UNITTEST_FOR(library/cpp/ssh_sign)

OWNER(lkozhinov)

SRCS(
    sign_ut.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
)

DATA(arcadia/library/cpp/ssh_sign/ut)

END()
