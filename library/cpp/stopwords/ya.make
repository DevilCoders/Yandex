LIBRARY()

OWNER(alzobnin)

PEERDIR(
    library/cpp/charset
    library/cpp/containers/str_map
    library/cpp/containers/str_hash
    library/cpp/wordlistreader
)

SRCS(
    stopwords.cpp
)

END()
