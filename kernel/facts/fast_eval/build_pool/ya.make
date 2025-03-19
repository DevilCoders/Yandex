PROGRAM()

OWNER(g:facts)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/facts/features_calculator
    kernel/facts/classifiers
    kernel/facts/factors_info
    library/cpp/getopt
    library/cpp/tokenizer
)

END()
