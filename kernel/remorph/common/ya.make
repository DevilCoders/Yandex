LIBRARY()

OWNER(g:remorph)

SRCS(
    common.cpp
    article_util.cpp
    magic_input.cpp
    reg_expr.cpp
    verbose.cpp
)

PEERDIR(
    contrib/libs/pcre
    kernel/gazetteer
    kernel/gazetteer/common
    kernel/geograph
    kernel/remorph/core
    library/cpp/containers/sorted_vector
    library/cpp/enumbitset
    library/cpp/regex/pcre
    library/cpp/solve_ambig
)

END()
