LIBRARY()

OWNER(shuster)

SRCS(
    docattrs.cpp
    docattrinserter.cpp
    dains.h
)

PEERDIR(
    kernel/groupattrs
    kernel/indexer/face
    kernel/keyinv/invkeypos
    kernel/search_types
    kernel/tarc/disk
    library/cpp/charset
    ysite/yandex/common
)

END()
