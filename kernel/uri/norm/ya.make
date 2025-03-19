OWNER(g:cpp-contrib)

LIBRARY()

PEERDIR(
    kernel/mirrors
    library/cpp/charset
    library/cpp/uri
    library/cpp/html/entity
    library/cpp/string_utils/quote
)

SRCS(
    conv_utf8.cpp
    flags.cpp
    frag.cpp
    host.cpp
    host_amzn.cpp
    host_goog.cpp
    host_spec.cpp
    host_twit.cpp
    host_wiki.cpp
    normalizer.cpp
    part.cpp
    path.cpp
    qry.cpp
)

END()
