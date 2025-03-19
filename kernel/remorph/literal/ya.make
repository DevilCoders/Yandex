LIBRARY()

OWNER(g:remorph)

SRCS(
    literal.cpp
    agreement.cpp
    lexer_names.cpp
    literal_table.cpp
    logic_expr.cpp
    ltelement.cpp
    parser.cpp
    lexer.l
)

PEERDIR(
    kernel/lemmer
    kernel/lemmer/dictlib
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/input
    library/cpp/charset
    library/cpp/containers/sorted_vector
    library/cpp/langmask
    library/cpp/regex/pcre
)

END()
