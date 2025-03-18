OWNER(g:geosearch)

LIBRARY()

CFLAGS(-D_LIB)

PEERDIR(
    contrib/libs/pugixml
    contrib/libs/protobuf
)

SRCS(
    pbfromxml.cpp
    map_field.cpp
)

END()
