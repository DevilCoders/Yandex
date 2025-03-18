OWNER(g:contrib g:cpp-contrib)

LIBRARY()

PEERDIR(
    contrib/libs/msgpack
)

SRCS(
    string_adaptor.cpp
    strbuf_adaptor.cpp
)

END()
