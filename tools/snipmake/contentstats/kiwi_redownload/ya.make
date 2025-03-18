PROGRAM()

OWNER(pzuev)

PEERDIR(
    library/cpp/getopt
    library/cpp/json
    contrib/libs/openssl
    yweb/structhtml/htmlstatslib
    tools/snipmake/contentstats/util
)

SRCS(
    kiwi_redownload.cpp
)

END()
