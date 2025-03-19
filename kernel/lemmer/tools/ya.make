LIBRARY()

OWNER(g:morphology)

SRCS(
    handler.cpp
)

PEERDIR(
    kernel/lemmer/core
    library/cpp/langmask
    library/cpp/tokenizer
    library/cpp/langs
)

END()
