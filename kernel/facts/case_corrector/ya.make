LIBRARY()

OWNER(g:facts)

PEERDIR(
    library/cpp/containers/comptrie
    quality/functionality/facts/tools/sentence_split/cpp
)

SRCS(
    case_corrector.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
