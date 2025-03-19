LIBRARY()

OWNER(g:remorph)

SRCS(
    richtree.cpp
    gzt_result_iter.cpp
    node_input_symbol.cpp
    node_input_symbol_factory.cpp
)

PEERDIR(
    kernel/gazetteer/richtree
    kernel/lemmer
    kernel/qtree/richrequest
    kernel/remorph/common
    kernel/remorph/input
    kernel/remorph/proc_base
    library/cpp/containers/sorted_vector
    library/cpp/langmask
)

END()
