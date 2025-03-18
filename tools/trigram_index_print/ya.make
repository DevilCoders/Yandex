OWNER(g:jupiter)

PROGRAM(trigram_index_print)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/doc_remap
    kernel/segmentator
    kernel/tarc/disk
    library/cpp/getopt
    ysite/yandex/trigram_index
)

END()
