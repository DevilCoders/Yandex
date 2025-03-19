LIBRARY()

OWNER(g:remorph)

SRCS(
    core.cpp
    compiler_impl.cpp
    debug.cpp
    parser_impl.cpp
)

PEERDIR(
    library/cpp/containers/sorted_vector
)

END()

