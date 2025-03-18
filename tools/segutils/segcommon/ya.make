LIBRARY()

OWNER(velavokr)

PEERDIR(
    kernel/hosts/owner
    kernel/segutils
    kernel/segutils_dater
    kernel/tarc/disk
    kernel/tarc/iface
    kernel/tarc/markup_zones
    library/cpp/cgiparam
    library/cpp/deprecated/dater_old
    library/cpp/deprecated/dater_old/scanner
    library/cpp/html/face
    library/cpp/html/spec
    library/cpp/http/fetch
    library/cpp/http/misc
    library/cpp/numerator
    yweb/news/fetcher_lib
)

SRCS(
    unpacker.cpp
    date_scanner.rl6
    arcutils.cpp
    qutils.cpp
    data_utils.cpp
    dumphelper.cpp
)

END()
