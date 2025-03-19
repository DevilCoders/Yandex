LIBRARY()

OWNER(g:morphology)

PEERDIR(
    library/cpp/string_utils/base64
    library/cpp/charset
    library/cpp/enumbitset
    library/cpp/langmask
    library/cpp/token
    library/cpp/langs
)

SRCS(
    ccl.cpp
    gleiche.cpp
    grambitset.cpp
    grammar_enum.cpp
    grammar_index.cpp
    terminals.cpp
    tgrammar_processing.cpp
)

END()
