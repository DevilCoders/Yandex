UNITTEST_FOR(kernel/facts/edit_distance_features)

OWNER(g:facts)

ALLOCATOR(LF)

SRCS(
    weighted_levenstein_ut.cpp
    transpose_words_ut.cpp
)

END()

NEED_CHECK()
