LIBRARY()

OWNER(g:morphology)

SRCS(
    tur.cpp
)

PEERDIR(
    kernel/lemmer/core
    kernel/lemmer/dictlib
    kernel/lemmer/new_dict/common
    library/cpp/tokenizer
)

END()
