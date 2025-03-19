LIBRARY()

OWNER(g:remorph)

SRCS(
    tokenizer.cpp
    remorph_tokenizer.cpp
)

PEERDIR(
    kernel/remorph/common
    library/cpp/charset
    library/cpp/enumbitset
    library/cpp/langmask
    library/cpp/token
    library/cpp/tokenizer
)

END()
