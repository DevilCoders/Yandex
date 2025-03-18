UNITTEST(dictionary_ut)

OWNER(
    g:matrixnet
    nikitxskv
)

SRCS(
    dictionary_ut.cpp
)

PEERDIR(
    library/cpp/text_processing/dictionary
    library/cpp/threading/local_executor
    library/cpp/testing/unittest
)

END()
