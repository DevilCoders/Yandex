UNITTEST()

# WARNING: do not use UNITTEST_FOR() or ADDINCL(library/mms)
# because of "string.h" header problem.

OWNER(sobols)

PEERDIR(
    library/cpp/on_disk/mms
)

SRCS(
    align_tools.h
    mms_cast_test.cpp
    mms_diff_test.cpp
    mms_example.cpp
    mms_hash_test.cpp
    mms_map_test.cpp
    mms_mapping_assignable_test.cpp
    mms_mapping_copyable_test.cpp
    mms_maybe_test.cpp
    mms_set_test.cpp
    mms_size_introspect_test.cpp
    mms_string_comparator_test.cpp
    mms_string_test.cpp
    mms_struct_test.cpp
    mms_vector_test.cpp
    mms_version_test.cpp
    tools.h
    write_to_blob.h
)

END()
