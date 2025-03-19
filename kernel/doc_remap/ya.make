OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    remap_reader.cpp
    index_url_extractor.cpp
    archive_url_extractor.cpp
    url_dat_remapper.cpp
    url_to_positions.cpp
    doc_id_iterator.cpp
    attr_url_extractor.h
    id2string.h
)

PEERDIR(
    kernel/groupattrs
    kernel/keyinv/indexfile
    kernel/tarc/disk
    library/cpp/containers/comptrie
    library/cpp/deprecated/fgood
    library/cpp/microbdb
    library/cpp/on_disk/chunks
    library/cpp/sorter
    library/cpp/string_utils/old_url_normalize
    library/cpp/wordpos
    yweb/robot/dbscheeme
)

END()
