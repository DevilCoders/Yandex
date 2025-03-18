OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    md5numerate.h
    decoder.cpp
    numerate.cpp
)

PEERDIR(
    library/cpp/charset
    library/cpp/containers/str_hash
    library/cpp/digest/md5
    library/cpp/html/entity
    library/cpp/html/face
    library/cpp/html/storage
    library/cpp/langs
    library/cpp/token
    library/cpp/tokenizer
)

END()
