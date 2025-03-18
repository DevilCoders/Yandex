PROGRAM()

PEERDIR(
    kernel/lemmer
    library/cpp/getopt
    library/cpp/html/face
    library/cpp/html/pdoc
    library/cpp/http/io
    library/cpp/uri
    ysite/yandex/spamfilt
)

SRCDIR(tools/printdocstat)

SRCS(
    printdocstat.cpp
)

END()
