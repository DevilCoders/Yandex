OWNER(g:cpp-contrib)

LIBRARY()

PEERDIR(
    kernel/matrixnet
    library/cpp/on_disk/aho_corasick
)

SRCS(ngrams_processor.cpp)

END()
