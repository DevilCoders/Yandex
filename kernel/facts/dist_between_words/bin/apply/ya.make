OWNER(nkireev)

PROGRAM(calc_dist)

PEERDIR(
    kernel/facts/dist_between_words
    kernel/normalize_by_lemmas
    library/cpp/tokenizer
    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()
