UNITTEST()

OWNER(
    g:factordev
)

PEERDIR(
    kernel/factor_storage
    kernel/factors_info/all_slices_infos
    kernel/factors_selector
    library/cpp/protobuf/util
)

SRCS(
    factors.cpp
)

END()
