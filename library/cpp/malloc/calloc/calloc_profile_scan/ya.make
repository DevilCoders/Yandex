PROGRAM()

OWNER(g:market-search)

PEERDIR(
    library/cpp/getopt
    library/cpp/on_disk/mms
)

GENERATE_ENUM_SERIALIZATION(process_state.h)

SRCS(calloc_profile_scan.cpp)

END()
