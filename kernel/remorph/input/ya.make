LIBRARY()

OWNER(g:remorph)

SRCS(
    input.cpp
    ctx_lemmas.cpp
    input_symbol.cpp
    input_symbol_util.cpp
    lemma_quality.cpp
    properties.cpp
    repr.cpp
    wtroka_input_symbol.cpp
)

PEERDIR(
    kernel/gazetteer
    kernel/gazetteer/common
    kernel/geograph
    kernel/inflectorlib/phrase
    kernel/lemmer
    kernel/lemmer/dictlib
    kernel/remorph/common
    kernel/remorph/core
    library/cpp/containers/sorted_vector
    library/cpp/deprecated/iter
    library/cpp/enumbitset
    library/cpp/langmask
    library/cpp/langs
)

GENERATE_ENUM_SERIALIZATION(lemma_quality.h)

END()
