LIBRARY()

OWNER(
    ilnurkh
    g:begemot
)

SRCS(
    component_api.cpp
    float_packing.cpp
)

PEERDIR(
    kernel/knn_service/protos
    quality/ytlib/tools/nodeio
    library/cpp/string_utils/base64
    library/cpp/dot_product
    library/cpp/cgiparam
    library/cpp/yson/node
    search/idl
)

END()
