LIBRARY()

OWNER(
    nsofya
    g:base
)

SRCS(
    searcharc.cpp
    unpacker.cpp
    wad_text_archive.cpp
    GLOBAL arcdir.cpp
    GLOBAL blob_archive.cpp
    GLOBAL file_archive.cpp
    GLOBAL map_archive.cpp
    GLOBAL multipart_archive.cpp
)

PEERDIR(
    kernel/doom/blob_storage
    kernel/doom/erasure
    kernel/doom/wad
    kernel/extarc_compression
    kernel/keyinv/invkeypos
    kernel/multipart_archive
    kernel/multipart_archive/config
    kernel/tarc/dirconf
    kernel/tarc/docdescr
    kernel/tarc/enums
    kernel/tarc/iface
    kernel/tarc/repack
    kernel/tarc/markup_zones
    kernel/tarc/protos
    kernel/xref
    library/cpp/archive
    library/cpp/charset
    library/cpp/containers/mh_heap
    library/cpp/deprecated/dater_old/date_attr_def
    library/cpp/langs
    library/cpp/mime/types
    library/cpp/object_factory
    library/cpp/packers
    library/cpp/string_utils/base64
    library/cpp/string_utils/old_url_normalize
    library/cpp/threading/future
    robot/jupiter/protos/extarc
    util/draft
)

END()
