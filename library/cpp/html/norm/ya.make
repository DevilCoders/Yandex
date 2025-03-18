LIBRARY()

OWNER(stanly)

SRCS(
    norm.cpp
)

PEERDIR(
    library/cpp/charset
    library/cpp/html/entity
    library/cpp/html/face
    library/cpp/html/lexer
    library/cpp/html/spec
    library/cpp/html/storage
    library/cpp/html/url
    library/cpp/logger
    library/cpp/uri
    library/cpp/xml/encode
)

END()
