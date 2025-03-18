PROGRAM()

WERROR()

OWNER(divankov)

PEERDIR(
    kernel/qtree/richrequest
    kernel/snippets/idl
    kernel/snippets/iface/archive
    kernel/tarc/iface
    kernel/walrus
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
    library/cpp/svnversion
    tools/snipmake/argv
    tools/snipmake/fatinv
    tools/snipmake/rawhits
    yweb/robot/kiwi/clientlib
)

SRCS(
    main.cpp
)

END()
