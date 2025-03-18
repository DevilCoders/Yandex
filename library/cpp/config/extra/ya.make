LIBRARY()

OWNER(
    pg
    velavokr
)

PEERDIR(
    library/cpp/scheme
    library/cpp/yconf
    library/cpp/config
    library/cpp/string_utils/relaxed_escaper
)

SRCS(
    yconf.cpp
)

END()
