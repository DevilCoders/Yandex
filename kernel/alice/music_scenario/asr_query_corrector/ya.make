LIBRARY()

OWNER(
    hommforever
    zychkamu
    g:alice_quality
)

SRCS(
    asr_query_corrector.cpp
)

PEERDIR(
    library/cpp/string_utils/levenshtein_diff
    kernel/translit
)

END()
