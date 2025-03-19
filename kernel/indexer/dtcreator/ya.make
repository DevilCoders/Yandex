LIBRARY()

OWNER(shuster)

SRCS(
    dtconfig.h
    dtcreator.cpp
    dthandler.cpp
    lemcache.cpp
    tokenproc.h
)

PEERDIR(
    kernel/indexer/direct_text
    kernel/indexer/face
    kernel/keyinv/invkeypos
    kernel/lemmer
    kernel/search_types
    library/cpp/langmask
    library/cpp/langs
    library/cpp/numerator
    library/cpp/token
    library/cpp/wordpos
    ysite/yandex/common
    ysite/yandex/pure
)

END()
