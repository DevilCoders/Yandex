UNITTEST()

OWNER(g:remorph)

SRCS(
    engine_char_ut.cpp
    remorph_ut.cpp
    tokenizer_ut.cpp
    tokenlogic_ut.cpp
)

PEERDIR(
    kernel/remorph/text
    kernel/remorph/tokenizer
    kernel/remorph/tokenlogic
    library/cpp/containers/sorted_vector
)

END()
