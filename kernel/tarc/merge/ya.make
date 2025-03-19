LIBRARY()

# Extracted from ysite/yandex/tarc

OWNER(
    g:base
    nsofya
)

PEERDIR(
    kernel/multipart_archive
    kernel/tarc/iface
    library/cpp/deprecated/mbitmap
    ysite/yandex/common
)

SRCS(
    GLOBAL flat_builder.cpp
    GLOBAL multipart_builder.cpp
    merge.cpp
)

END()
