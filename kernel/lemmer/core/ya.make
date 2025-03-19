LIBRARY()

OWNER(
    g:base
    g:morphology
)

PEERDIR(
    kernel/search_types
    library/cpp/charset
    library/cpp/langmask
    kernel/lemmer/alpha
    kernel/lemmer/context/default_decimator
    kernel/lemmer/dictlib
    kernel/lemmer/registry
    kernel/lemmer/translate
    kernel/lemmer/untranslit
    library/cpp/stopwords
    library/cpp/tokenizer
    library/cpp/wordlistreader
    library/cpp/token
    library/cpp/langs
)

SRCS(
    analyze_word.cpp
    wordform.cpp
    decimator.cpp
    disamb_options.cpp
    formgenerator.cpp
    langcontext.cpp
    language.cpp
    language_translit.cpp
    lemmaforms.cpp
    lemmer.cpp
    morphfixlist.cpp
    morpho_lang_discr.cpp
    options.cpp
    rubbishdump.cpp
    token.cpp
    wordinstance.cpp
    wordinstance_update.cpp
)

END()
