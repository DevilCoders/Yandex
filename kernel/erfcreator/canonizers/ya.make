OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    canonizers.cpp
)

PEERDIR(
    library/cpp/string_utils/url
    kernel/url
    kernel/erfcreator/common_config
)

END()
