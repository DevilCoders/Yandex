UNITTEST_FOR(kernel/facts/word_embedding)

OWNER(g:facts)

SRCS(
    word_embedding_ut.cpp
)

RESOURCE(
   dict.words dict_words
   dict.vectors dict_vectors
)

END()
