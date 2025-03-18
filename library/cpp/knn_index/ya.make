LIBRARY()

OWNER(kcd)

SRCS(
    all.cpp
    constants.h
    distance.h
    document.h
    index_info.proto
    item.h
    search_item_result.h
    searcher.h
    writer.h
)

GENERATE_ENUM_SERIALIZATION(distance.h)

PEERDIR(
    library/cpp/containers/limited_heap
    library/cpp/dot_product
    library/cpp/l1_distance
    library/cpp/l2_distance
)

END()
