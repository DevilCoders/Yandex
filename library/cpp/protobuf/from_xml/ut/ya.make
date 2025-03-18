OWNER(g:geosearch)

UNITTEST()

PEERDIR(
    contrib/libs/pugixml
    library/cpp/protobuf/from_xml
)

DATA(
    arcadia/library/cpp/protobuf/from_xml/ut/data/recursive.xml
    arcadia/library/cpp/protobuf/from_xml/ut/data/recursive.pb
    arcadia/library/cpp/protobuf/from_xml/ut/data/data_types.xml
    arcadia/library/cpp/protobuf/from_xml/ut/data/data_types.pb
)

SRCS(
    node_text_ut.cpp
    pb_field_wrapper_ut.cpp
    field_mapping_ut.cpp
    map_field_ut.cpp
    pbfromxml_ut.cpp
    proto/basic.proto
    proto/recursive.proto
    proto/data_types.proto
)

END()
