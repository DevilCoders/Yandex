LIBRARY()

OWNER(mvel)

PEERDIR(
    kernel/groupattrs
    library/cpp/deprecated/fgood
    library/cpp/deprecated/mbitmap
    ysite/yandex/common
)

SRCS(
    creator.cpp
    mergeattrs.cpp
    metacreator.cpp
)

END()
