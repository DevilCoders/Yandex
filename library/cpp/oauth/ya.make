LIBRARY()

OWNER(achigin)

SRCS(
    oauth.cpp
)

PEERDIR(
    library/cpp/http/simple
    library/cpp/json
    library/cpp/ssh_sign
    library/cpp/string_utils/base64
)

END()
