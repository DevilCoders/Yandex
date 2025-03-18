OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    blob_reader.cpp
    eliasfano.h
    eliasfanomonotone.h
    fredrikssonnikitin.h
    freqvaldict.h
    intvector.h
    rankselect.cpp
)

PEERDIR(
    library/cpp/containers/bitseq
    library/cpp/pop_count
    library/cpp/select_in_word
)

END()
