LIBRARY()

OWNER(g:morphology)

PEERDIR(
    kernel/lemmer/alpha
    kernel/lemmer/untranslit/trie
    library/cpp/langmask
)

SRCS(
    ${ARCADIA_ROOT}/kernel/lemmer/untranslit/untranslit.cpp
    ${ARCADIA_ROOT}/kernel/lemmer/untranslit/ngrams/ngrams.cpp
)

END()
