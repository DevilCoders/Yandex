LIBRARY()

OWNER(
    g:base
    g:factordev
)

PEERDIR(
    kernel/factors_info
    kernel/generated_factors_info/metadata
    library/cpp/json
)

SRCS(simple_factors_info.cpp)

END()
