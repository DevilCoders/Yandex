PROGRAM()

OWNER(pzuev)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/hosts/owner
    library/cpp/getopt
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    tools/segutils/quality_measurers/segqualitycommon
    tools/segutils/segcommon
)

END()
