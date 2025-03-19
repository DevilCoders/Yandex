LIBRARY()

OWNER(
    mbusel
    g:factordev
)

PEERDIR(
    library/cpp/charset
    ysite/yandex/reqanalysis
    kernel/stringmatch_tracker/matchers
    kernel/translit
)

SRCS(
    lcs_wrapper.cpp
    trigram_wrappers.cpp
    translit_preparer.cpp
    tracker.cpp
    trigram_iterator.cpp
    prod_preparer.cpp
)

GENERATE_ENUM_SERIALIZATION(feature_description.h)

END()
