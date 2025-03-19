LIBRARY()

OWNER(g:snippets)

NO_WSHADOW()

SRCS(
    char_class.cpp
    clearchar.cpp
    consts.cpp
    cutparam.cpp
    hilited_length.cpp
    multi_length_cut.cpp
    pixel_length.cpp
    smartcut.cpp
    snip_length.cpp
    wordsinfo.cpp
    wordspan_length.cpp
)

PEERDIR(
    kernel/snippets/strhl
    library/cpp/stopwords
    library/cpp/token
    library/cpp/tokenizer
)

END()
