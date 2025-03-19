LIBRARY()

OWNER(alzobnin)

SRCS(
    htmlparse.cpp
    recshell.cpp
)

PEERDIR(
    dict/recognize/docrec
    library/cpp/charset
    library/cpp/html/entity
    library/cpp/html/face
    library/cpp/html/face/blob
    library/cpp/html/storage
    library/cpp/html/html5
    library/cpp/string_utils/url
)

END()
