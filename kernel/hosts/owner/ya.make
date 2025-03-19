OWNER(velavokr)

LIBRARY()

SRCS(
    owner.cpp
)

PEERDIR(
    library/cpp/archive
    library/cpp/containers/str_hash
    library/cpp/string_utils/url
)

SRCDIR(
    yweb/urlrules
)

ARCHIVE(
    NAME urlrules.inc
    areas.lst
    2ld.list
    ungrouped.list
)

END()
