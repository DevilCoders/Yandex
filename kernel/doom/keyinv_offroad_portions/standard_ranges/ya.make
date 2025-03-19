LIBRARY()

OWNER(
    g:base
)

SRCS(
    range_types.h
    standard_ranges.h
)

GENERATE_ENUM_SERIALIZATION(range_types.h)

RESOURCE(
    ranges/standard_keyinv_ranges_counts_keyinv_ranges standard_keyinv_ranges_counts_keyinv_ranges
    ranges/standard_keyinv_ranges_attributes_keyinv_ranges standard_keyinv_ranges_attributes_keyinv_ranges
    ranges/standard_keyinv_ranges_ngram_hashes_counts_keyinv_ranges standard_keyinv_ranges_ngram_hashes_counts_keyinv_ranges
    ranges/standard_keyinv_ranges_keyinv_keyinv_ranges standard_keyinv_ranges_keyinv_keyinv_ranges
    ranges/standard_keyinv_ranges_counts_keyinv_ranges_v2 standard_keyinv_ranges_counts_keyinv_ranges_v2
)

PEERDIR(
    kernel/doom/keyinv_offroad_portions
    library/cpp/resource
)

END()

