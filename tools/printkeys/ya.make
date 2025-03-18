PROGRAM()

OWNER(leo)

PEERDIR(
    kernel/keyinv/hitlist
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    kernel/search_types
    library/cpp/charset
    library/cpp/containers/mh_heap
    library/cpp/deprecated/autoarray
    library/cpp/getopt
    ysite/yandex/common
)

SRCS(
    printkeys.cc
)

IF ("${CMAKE_BUILD_TYPE}" MATCHES "Release")
    CFLAGS(-DPACKAGE)
ENDIF()

END()
