LIBRARY()

OWNER(g:remorph)

SRCS(
    syntax.cpp
    syntax_chunk.cpp
    syntax_chunk_builder.cpp
)

PEERDIR(
    kernel/lemmer/dictlib
    kernel/remorph/core
    kernel/remorph/input
    kernel/remorph/matcher
    library/cpp/enumbitset
    library/cpp/solve_ambig
)

END()
