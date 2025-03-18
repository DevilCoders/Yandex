LIBRARY()

OWNER(velavokr)

SRCS(
    idxr_all.cpp
)

PEERDIR(
    library/cpp/codecs
    library/cpp/containers/paged_vector
    library/cpp/deprecated/accessors
    library/cpp/string_utils/relaxed_escaper
    library/cpp/succinct_arrays
)

END()
