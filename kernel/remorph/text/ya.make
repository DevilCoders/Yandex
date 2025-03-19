LIBRARY()

OWNER(g:remorph)

SRCS(
    text.cpp
    word_input_symbol.cpp
    word_symbol_factory.cpp
)

PEERDIR(
    kernel/gazetteer
    kernel/lemmer
    kernel/lemmer/alpha
    kernel/lemmer/dictlib
    kernel/remorph/cascade
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/facts
    kernel/remorph/input
    kernel/remorph/matcher
    kernel/remorph/misc/ansi_escape
    kernel/remorph/tokenizer
    library/cpp/langmask
    library/cpp/solve_ambig
    library/cpp/token
    library/cpp/tokenizer
)

END()
