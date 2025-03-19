LIBRARY()

OWNER(
    otrok
    g:unstructured-data
)

SRCS(
    normalize.cpp
    special_words.gztproto
)

PEERDIR(
    kernel/gazetteer
    kernel/gazetteer/richtree
    kernel/gazetteer/simpletext
    kernel/lemmer/core
    kernel/remorph/input
    kernel/remorph/matcher
    library/cpp/token
    library/cpp/tokenizer
)

END()
