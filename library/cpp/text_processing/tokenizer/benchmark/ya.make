G_BENCHMARK()

OWNER(bulatman g:matrixnet)

SRCS(
    tokenizer_benchmark.cpp
)

PEERDIR(
    library/cpp/text_processing/tokenizer
    library/cpp/testing/common
)

END()
