LIBRARY()

OWNER(
    mvel
    kirillovs
    g:base
)

SRCS(
    idna_decode.cpp
    smart.cpp
    translit.cpp
    url_tools.cpp
    utf8.cpp
)

PEERDIR(
    contrib/libs/libidn
    kernel/lemmer/core
    kernel/lemmer/new_dict/rus
    kernel/lemmer/untranslit
    library/cpp/charset
    library/cpp/string_utils/url
    library/cpp/tld
    library/cpp/uri
)

END()
