LIBRARY()

OWNER(
    bogolubsky
    g:facts
)

SRCS(
    article_name_extractor.cpp
)

PEERDIR(
    util/charset
    kernel/normalize_by_lemmas
)

END()
