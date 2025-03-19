LIBRARY()

OWNER(
    sankear
    g:base
)

SRCS(
    check_sum_doc_lump.cpp
    deduplicator.cpp
    mapper.cpp
    mapper.h
    mega_wad.cpp
    mega_wad_common.cpp
    mega_wad_info.cpp
    mega_wad_info.proto
    mega_wad_merger.cpp
    multi_mapper.cpp
    wad.cpp
    wad_lump_id.cpp
    wad_router.cpp
    wad_writer.cpp
)

GENERATE_ENUM_SERIALIZATION(wad_lump_role.h)

GENERATE_ENUM_SERIALIZATION(wad_index_type.h)

GENERATE_ENUM_SERIALIZATION(wad_signature.h)

PEERDIR(
    library/cpp/offroad/custom
    library/cpp/offroad/utility
    library/cpp/offroad/wad
    kernel/doom/blob_storage
    kernel/doom/info
    library/cpp/protobuf/json
    library/cpp/digest/crc32c
)

END()
