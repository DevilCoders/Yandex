LIBRARY(edit_distance_features)

OWNER(g:facts)

PEERDIR(
    kernel/lemmer/core
    kernel/lemmer/new_dict/eng
    kernel/lemmer/new_dict/rus
    library/cpp/string_utils/levenshtein_diff
)

SRCS(
    weighted_levenstein.cpp
    transpose_words.cpp
    word_lemma_pair.cpp
)

END()
