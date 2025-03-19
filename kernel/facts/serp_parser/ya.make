LIBRARY()

OWNER(
    g:facts
)

SRCS(
    extract_answer.cpp
    extract_headline.cpp
    extract_url.cpp
)

PEERDIR(
    library/cpp/scheme
)

END()
