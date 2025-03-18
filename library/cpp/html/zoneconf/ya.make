LIBRARY()

OWNER(stanly)

PEERDIR(
    library/cpp/charset
    library/cpp/containers/str_hash
    library/cpp/html/entity
    library/cpp/html/face
    library/cpp/html/relalternate
    library/cpp/html/spec
    library/cpp/html/url
    library/cpp/http/misc
    library/cpp/uri
    library/cpp/yconf
)

SRCS(
    attrextractor.cpp
    findin.h
    ht_conf.cpp
    parsefunc.cpp
)

END()
