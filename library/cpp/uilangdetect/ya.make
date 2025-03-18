OWNER(g:cpp-contrib)

LIBRARY()

PEERDIR(
    library/cpp/string_utils/base64
    library/cpp/http/misc
    library/cpp/langs
    library/cpp/cgiparam
)

SRCS(
    uilangdetect.cpp
    mycookie.cpp
    bycookie.cpp
    bytld.cpp
    byacceptlang.cpp
)

END()
