PROGRAM()

OWNER(pzuev)

PEERDIR(
    kernel/snippets/archive/zone_checker
    kernel/snippets/base
    kernel/snippets/idl
    kernel/snippets/iface/archive
    kernel/tarc/markup_zones
    library/cpp/getopt
    library/cpp/html/pcdata
    library/cpp/string_utils/base64
    library/cpp/svnversion
    tools/snipmake/argv
    yweb/structhtml/htmlstatslib/runtime
)

SRCS(
    main.cpp
)

END()
