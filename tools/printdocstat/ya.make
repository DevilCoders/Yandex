PROGRAM()

PEERDIR(
    kernel/lemmer
    library/cpp/charset
    library/cpp/getopt
    library/cpp/html/face
    library/cpp/html/pdoc
    library/cpp/http/io
    library/cpp/uri
    ysite/yandex/spamfilt
)

SRCS(
    printdocstat.cpp
)

END()
