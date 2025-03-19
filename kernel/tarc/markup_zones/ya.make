LIBRARY()

OWNER(
    nsofya
    g:base
)

PEERDIR(
    kernel/keyinv/invkeypos
    kernel/segmentator/structs
    kernel/tarc/docdescr
    kernel/tarc/iface
    kernel/xref
    library/cpp/charset
    library/cpp/containers/mh_heap
    library/cpp/deprecated/dater_old/date_attr_def
    library/cpp/langs
)

SRCS(
    arcreader.cpp
    text_markup.cpp
    unpackers.cpp
    view_sent.cpp
    searcharc_common.cpp
)

END()
