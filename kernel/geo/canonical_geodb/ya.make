LIBRARY()
OWNER(g:lingboost)

INCLUDE(${ARCADIA_ROOT}/kernel/geo/canonical_data/files/geodata_tree_ling/ya.make.inc)
RESOURCE(geodata4-tree+ling.bin canonical_geodb)
PEERDIR(
    geobase/library
    library/cpp/resource
)
SRCS(geodata.cpp)

END()
