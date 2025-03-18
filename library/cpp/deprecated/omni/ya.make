LIBRARY()

OWNER(
    elric
    mvel
    g:base
)

PEERDIR(
    util
    library/cpp/json
    library/cpp/comptable
)

SRCS(
    atlas.cpp
    codecs.cpp
    common.cpp
    dbg_view.cpp
    read.cpp
    write.cpp
)

END()
