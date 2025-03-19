LIBRARY()

OWNER(
    g:base
    g:neural-search
    crossby
    tagrimar
    olegator
    tsimkha
    agusakov
    carzil
)

PEERDIR(
    library/cpp/containers/top_keeper
    library/cpp/protobuf/util
    library/cpp/dot_product
    kernel/dssm_applier
)

SRCS(
    aggregators.cpp
)

END()
