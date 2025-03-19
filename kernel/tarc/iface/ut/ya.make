UNITTEST_FOR(kernel/tarc/iface)

OWNER(g:base)

SRCS(
    from_string_ut.cpp
    tarcio_ut.cpp
)

PEERDIR(
    kernel/tarc/iface
    library/cpp/resource
)

RESOURCE(
    makearc_input_indexarc /input_indexarc
    makearc_canonical_indexdir /canonical_indexdir
)

END()
