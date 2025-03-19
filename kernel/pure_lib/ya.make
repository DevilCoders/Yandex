OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    formcalcer.cpp
    lemma_cache.cpp
)

PEERDIR(
    kernel/lemmer/alpha
    kernel/lemmer/core
    kernel/search_types
    library/cpp/tokenizer
    ysite/yandex/common
)

END()
