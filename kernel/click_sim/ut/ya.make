UNITTEST()

OWNER(
    tobo
    g:images-followers
    g:images-robot
    g:images-search-quality
    g:images-nonsearch-quality
)

SRCS(
    binarized_word_vector_ut.cpp
    word_vector_ut.cpp
)

PEERDIR(
    kernel/click_sim
    library/cpp/vowpalwabbit
)

END()
