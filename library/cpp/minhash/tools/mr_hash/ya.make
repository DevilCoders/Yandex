PROGRAM()

OWNER(zador)

SRCS(
    main.cpp
    mr_hash.cpp
)

PEERDIR(
    library/cpp/getopt
    library/cpp/minhash
    library/cpp/streams/factory
    mapreduce/lib
    mapreduce/library/seq_calc
    mapreduce/library/temptable
)

END()
