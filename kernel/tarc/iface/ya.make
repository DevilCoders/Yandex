LIBRARY()

OWNER(
    nsofya
    g:base
)

SRCS(
    arcface.cpp
    farcface.cpp
    tarcface.cpp
    tarcio.cpp
    settings.cpp
    zones.gperf
    comp_tables.cpp
)

PEERDIR(
    contrib/libs/protobuf
    kernel/segmentator/structs
    kernel/tarc/enums
    kernel/tarc/protos
    library/cpp/archive
    library/cpp/comptable
    library/cpp/containers/comptrie
    library/cpp/packers
)

ARCHIVE(
    NAME comp_tables.inc
    comp_table_markup_zones.bin
    comp_table_sent_infos.bin
)

END()
