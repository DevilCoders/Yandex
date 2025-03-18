UNITTEST()

OWNER(
    stanly
    nsofya
)

SRCS(
    broken_encodings_ut.cpp
    noindex_ut.cpp
    tags_ut.cpp
    text_normalize_ut.cpp
    twitter_ut.cpp
)

PEERDIR(
    library/cpp/html/html5
    library/cpp/html/storage
)

END()
