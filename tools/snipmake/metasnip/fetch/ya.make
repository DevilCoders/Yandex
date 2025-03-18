PROGRAM(metasnip-fetch)

OWNER(divankov)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    library/cpp/charset
    library/cpp/getopt
    tools/snipmake/metasnip/jobqueue
    library/cpp/http/io
    library/cpp/cgiparam
    library/cpp/string_utils/quote
)

END()
