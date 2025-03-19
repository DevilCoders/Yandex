LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/factor_storage
    kernel/relevfml
    kernel/snippets/factors
    kernel/snippets/formulae
    kernel/snippets/idl
    kernel/snippets/iface
    kernel/web_factors_info
    library/cpp/codecs
    library/cpp/langmask/serialization
    library/cpp/scheme
    library/cpp/stopwords
    search/idl
)

SRCS(
    config.cpp
    enums.cpp
)

END()
